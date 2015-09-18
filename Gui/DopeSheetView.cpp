/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2015 INRIA and Alexandre Gauthier-Foichat
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

// warning: 'gluErrorString' is deprecated: first deprecated in OS X 10.9 [-Wdeprecated-declarations]
CLANG_DIAG_OFF(deprecated-declarations)
GCC_DIAG_OFF(deprecated-declarations)

namespace {
//Protect declarations in an anonymous namespace

// Typedefs
typedef std::set<double> TimeSet;
typedef std::pair<double, double> FrameRange;
typedef std::map<boost::weak_ptr<KnobI>, KnobGui *> KnobsAndGuis;


// Constants
static const int KF_TEXTURES_COUNT = 18;
static const int KF_PIXMAP_SIZE = 14;
static const int KF_X_OFFSET = KF_PIXMAP_SIZE / 2;
static const int DISTANCE_ACCEPTANCE_FROM_KEYFRAME = 5;
static const int DISTANCE_ACCEPTANCE_FROM_READER_EDGE = 14;
static const int DISTANCE_ACCEPTANCE_FROM_READER_BOTTOM = 8;


////////////////////////// Helpers //////////////////////////


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

} // anon namespace


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

        kfTextureMaster,
        kfTextureMasterSelected,
    };

    DopeSheetViewPrivate(DopeSheetView *qq);
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

    std::vector<DopeSheetKey> isNearByKeyframe(const boost::shared_ptr<DSKnob> &dsKnob, const QPointF &widgetCoords) const;
    std::vector<DopeSheetKey> isNearByKeyframe(boost::shared_ptr<DSNode> dsNode, const QPointF &widgetCoords) const;

    double clampedMouseOffset(double fromTime, double toTime);

    // Textures
    void generateKeyframeTextures();
    DopeSheetViewPrivate::KeyframeTexture kfTextureFromKeyframeType(Natron::KeyframeTypeEnum kfType, bool selected) const;

    // Drawing
    void drawScale() const;

    void drawRows() const;

    void drawNodeRow(const boost::shared_ptr<DSNode> dsNode) const;
    void drawKnobRow(const boost::shared_ptr<DSKnob> dsKnob) const;

    void drawNodeRowSeparation(const boost::shared_ptr<DSNode> dsNode) const;

    void drawRange(const boost::shared_ptr<DSNode> &dsNode) const;
    void drawKeyframes(const boost::shared_ptr<DSNode> &dsNode) const;

    void drawTexturedKeyframe(DopeSheetViewPrivate::KeyframeTexture textureType,
                              bool drawTime,
                              int time,
                              const QColor& textColor,
                              const RectD &rect) const;

    void drawGroupOverlay(const boost::shared_ptr<DSNode> &dsNode, const boost::shared_ptr<DSNode> &group) const;

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

    void computeRangesBelow(DSNode * dsNode);
    void computeNodeRange(DSNode *dsNode);
    void computeReaderRange(DSNode *reader);
    void computeRetimeRange(DSNode *retimer);
    void computeTimeOffsetRange(DSNode *timeOffset);
    void computeFRRange(DSNode *frameRange);
    void computeGroupRange(DSNode *group);

    // User interaction
    void onMouseLeftButtonDrag(QMouseEvent *e);

    void createSelectionFromRect(const RectD &rect, std::vector<DopeSheetKey> *result, std::vector<boost::shared_ptr<DSNode> >* selectedNodes);

    void moveCurrentFrameIndicator(double dt);

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
    GLuint kfTexturesIDs[KF_TEXTURES_COUNT];

    // for navigating
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
    boost::shared_ptr<DSNode> currentEditedReader;

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
    kfTexturesIDs(),
    zoomContext(),
    zoomOrPannedSinceLastFit(false),
    selectionRect(),
    selectedKeysBRect(),
    lastPosOnMousePress(),
    lastPosOnMouseMove(),
    keyDragLastMovement(),
    eventState(DopeSheetView::esNoEditingState),
    currentEditedReader(),
    hasOpenGLVAOSupport(true),
    contextMenu(new Natron::Menu(q_ptr))
{

}

DopeSheetViewPrivate::~DopeSheetViewPrivate()
{
    glDeleteTextures(KF_TEXTURES_COUNT, kfTexturesIDs);
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
DopeSheetViewPrivate::getKeyFrameBoundingRectZoomCoords(double keyframeTimeZoomCoords, double keyframeYCenterWidgetCoords) const
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

/*
 QRectF and Qt coordinate system has its y axis top-down, whereas in Natron
 the coordinate system is bottom-up. When using QRectF, top-left is in fact (0,0)
 while in Natron it is (0,h - 1).
 Hence we use different data types to identify the 2 different coordinate systems.
 */
RectD DopeSheetViewPrivate::rectToZoomCoordinates(const QRectF &rect) const
{
    QPointF topLeft = rect.topLeft();
    topLeft = zoomContext.toZoomCoordinates(topLeft.x(), topLeft.y());
    RectD ret;
    ret.x1 = topLeft.x();
    ret.y2 = topLeft.y();
    QPointF bottomRight = rect.bottomRight();
    bottomRight = zoomContext.toZoomCoordinates(bottomRight.x(), bottomRight.y());
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
QRectF DopeSheetViewPrivate::rectToWidgetCoordinates(const RectD &rect) const
{
    QPointF topLeft = zoomContext.toWidgetCoordinates(rect.x1, rect.y2);
    QPointF bottomRight = zoomContext.toWidgetCoordinates(rect.x2, rect.y1);
    return QRectF(topLeft.x(), topLeft.y(), bottomRight.x() - topLeft.x(), bottomRight.y() - topLeft.y());
}

QRectF DopeSheetViewPrivate::nameItemRectToRowRect(const QRectF &rect) const
{
    QPointF topLeft = rect.topLeft();
    topLeft = zoomContext.toZoomCoordinates(topLeft.x(), topLeft.y());
    QPointF bottomRight = rect.bottomRight();
    bottomRight = zoomContext.toZoomCoordinates(bottomRight.x(), bottomRight.y());

    return QRectF(QPointF(zoomContext.left(), topLeft.y()),
                  QPointF(zoomContext.right(), bottomRight.y()));
}

Qt::CursorShape DopeSheetViewPrivate::getCursorDuringHover(const QPointF &widgetCoords) const
{
    QPointF clickZoomCoords = zoomContext.toZoomCoordinates(widgetCoords.x(), widgetCoords.y());

    if (isNearbySelectedKeysBRec(widgetCoords)) {
        return getCursorForEventState(DopeSheetView::esMoveKeyframeSelection);
    } else if (isNearbySelectedKeysBRectRightEdge(widgetCoords)) {
        return getCursorForEventState(DopeSheetView::esTransformingKeyframesMiddleRight);
    } else if (isNearbySelectedKeysBRectLeftEdge(widgetCoords)) {
        return getCursorForEventState(DopeSheetView::esTransformingKeyframesMiddleLeft);
    } else if (isNearByCurrentFrameIndicatorBottom(clickZoomCoords)) {
        return getCursorForEventState(DopeSheetView::esMoveCurrentFrameIndicator);
    }
    
    ///Look for a range node
    DSTreeItemNodeMap nodes = model->getItemNodeMap();
    for (DSTreeItemNodeMap::iterator it = nodes.begin(); it!=nodes.end(); ++it) {
        if (it->second->isRangeDrawingEnabled()) {
            
            std::map<DSNode *, FrameRange >::const_iterator foundRange = nodeRanges.find(it->second.get());
            if (foundRange == nodeRanges.end()) {
                continue;
            }
            
            QRectF treeItemRect = hierarchyView->visualItemRect(it->first);
            
            const FrameRange& range = foundRange->second;
            RectD nodeClipRect;
            nodeClipRect.x1 = range.first;
            nodeClipRect.x2 = range.second;
            nodeClipRect.y2 = zoomContext.toZoomCoordinates(0, treeItemRect.top()).y();
            nodeClipRect.y1 = zoomContext.toZoomCoordinates(0, treeItemRect.bottom()).y();
            
            Natron::DopeSheetItemType nodeType = it->second->getItemType();
            if (nodeClipRect.contains(clickZoomCoords.x(), clickZoomCoords.y())) {
                if (nodeType == Natron::eDopeSheetItemTypeGroup ||
                    nodeType == Natron::eDopeSheetItemTypeReader ||
                    nodeType == Natron::eDopeSheetItemTypeTimeOffset ||
                    nodeType == Natron::eDopeSheetItemTypeFrameRange) {
                    return getCursorForEventState(DopeSheetView::esMoveKeyframeSelection);
                }
            }
            if (nodeType == Natron::eDopeSheetItemTypeReader) {
                if (isNearByClipRectLeft(clickZoomCoords, nodeClipRect)) {
                    return getCursorForEventState(DopeSheetView::esReaderLeftTrim);
                } else if (isNearByClipRectRight(clickZoomCoords, nodeClipRect)) {
                    return getCursorForEventState(DopeSheetView::esReaderRightTrim);
                } else if (model->canSlipReader(it->second) && isNearByClipRectBottom(clickZoomCoords, nodeClipRect)) {
                    return getCursorForEventState(DopeSheetView::esReaderSlip);
                }
            }
            
        }
    } // for (DSTreeItemNodeMap::iterator it = nodes.begin(); it!=nodes.end(); ++it) {

    QTreeWidgetItem *treeItem = hierarchyView->itemAt(0, widgetCoords.y());
    //Did not find a range node, look for keyframes
    if (treeItem) {
        DSTreeItemNodeMap dsNodeItems = model->getItemNodeMap();
        DSTreeItemNodeMap::const_iterator foundDsNode = dsNodeItems.find(treeItem);
        if (foundDsNode != dsNodeItems.end()) {
            const boost::shared_ptr<DSNode> &dsNode = (*foundDsNode).second;
            Natron::DopeSheetItemType nodeType = dsNode->getItemType();
            if (nodeType == Natron::eDopeSheetItemTypeCommon) {
                std::vector<DopeSheetKey> keysUnderMouse = isNearByKeyframe(dsNode, widgetCoords);
                
                if (!keysUnderMouse.empty()) {
                    return getCursorForEventState(DopeSheetView::esPickKeyframe);
                }
                
            }
        } else { // if (foundDsNode != dsNodeItems.end()) {
            //We may be on a knob row
            boost::shared_ptr<DSKnob> dsKnob = hierarchyView->getDSKnobAt(widgetCoords.y());
            if (dsKnob) {
                std::vector<DopeSheetKey> keysUnderMouse = isNearByKeyframe(dsKnob, widgetCoords);
                
                if (!keysUnderMouse.empty()) {
                    return getCursorForEventState(DopeSheetView::esPickKeyframe);
                }
            }
            
        }
    } // if (treeItem) {

    return getCursorForEventState(DopeSheetView::esNoEditingState);
}

Qt::CursorShape DopeSheetViewPrivate::getCursorForEventState(DopeSheetView::EventStateEnum es) const
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

bool DopeSheetViewPrivate::isNearByClipRectLeft(const QPointF& zoomCoordPos, const RectD &clipRect) const
{
    QPointF widgetPos = zoomContext.toWidgetCoordinates(zoomCoordPos.x(),zoomCoordPos.y());
    QPointF rectx1y1 = zoomContext.toWidgetCoordinates(clipRect.left(), clipRect.bottom());
    QPointF rectx2y2 = zoomContext.toWidgetCoordinates(clipRect.right(), clipRect.top());
    
    return ((widgetPos.x() >= rectx1y1.x() - DISTANCE_ACCEPTANCE_FROM_READER_EDGE) &&
            (widgetPos.x() <= rectx1y1.x()) &&
            (widgetPos.y() <= rectx1y1.y() + DISTANCE_ACCEPTANCE_FROM_READER_EDGE) &&
            (widgetPos.y() >= rectx2y2.y() - DISTANCE_ACCEPTANCE_FROM_READER_EDGE));
}

bool DopeSheetViewPrivate::isNearByClipRectRight(const QPointF& zoomCoordPos, const RectD &clipRect) const
{
    QPointF widgetPos = zoomContext.toWidgetCoordinates(zoomCoordPos.x(),zoomCoordPos.y());
    QPointF rectx1y1 = zoomContext.toWidgetCoordinates(clipRect.left(), clipRect.bottom());
    QPointF rectx2y2 = zoomContext.toWidgetCoordinates(clipRect.right(), clipRect.top());
    
    return ((widgetPos.x() >= rectx2y2.x()) &&
            (widgetPos.x() <= rectx2y2.x() + DISTANCE_ACCEPTANCE_FROM_READER_EDGE) &&
            (widgetPos.y() <= rectx1y1.y() + DISTANCE_ACCEPTANCE_FROM_READER_EDGE) &&
            (widgetPos.y() >= rectx2y2.y() - DISTANCE_ACCEPTANCE_FROM_READER_EDGE));
}

bool DopeSheetViewPrivate::isNearByClipRectBottom(const QPointF& zoomCoordPos, const RectD &clipRect) const
{
    QPointF widgetPos = zoomContext.toWidgetCoordinates(zoomCoordPos.x(),zoomCoordPos.y());
    QPointF rectx1y1 = zoomContext.toWidgetCoordinates(clipRect.left(), clipRect.bottom());
    QPointF rectx2y2 = zoomContext.toWidgetCoordinates(clipRect.right(), clipRect.top());
    
    return ((widgetPos.x() >= rectx1y1.x() - DISTANCE_ACCEPTANCE_FROM_READER_BOTTOM) &&
            (widgetPos.x() <= rectx2y2.x() + DISTANCE_ACCEPTANCE_FROM_READER_EDGE) &&
            (widgetPos.y() <= rectx1y1.y() + DISTANCE_ACCEPTANCE_FROM_READER_BOTTOM) &&
            (widgetPos.y() >= rectx1y1.y() - DISTANCE_ACCEPTANCE_FROM_READER_BOTTOM));

}

bool DopeSheetViewPrivate::isNearByCurrentFrameIndicatorBottom(const QPointF &zoomCoords) const
{
    return (currentFrameIndicatorBottomPoly.containsPoint(zoomCoords, Qt::OddEvenFill));
}

bool
DopeSheetViewPrivate::isNearbySelectedKeysBRectRightEdge(const QPointF& widgetPos) const
{
    QPointF topLeft = zoomContext.toWidgetCoordinates(selectedKeysBRect.x1, selectedKeysBRect.y2);
    QPointF bottomRight = zoomContext.toWidgetCoordinates(selectedKeysBRect.x2, selectedKeysBRect.y1);
    
    return (widgetPos.x() >= (bottomRight.x() ) &&
            widgetPos.x() <= (bottomRight.x() + DISTANCE_ACCEPTANCE_FROM_READER_EDGE) &&
            widgetPos.y() <= (bottomRight.y() + DISTANCE_ACCEPTANCE_FROM_READER_EDGE) &&
            widgetPos.y() >= (topLeft.y() - DISTANCE_ACCEPTANCE_FROM_READER_EDGE));
}

bool
DopeSheetViewPrivate::isNearbySelectedKeysBRectLeftEdge(const QPointF& widgetPos) const
{
    QPointF topLeft = zoomContext.toWidgetCoordinates(selectedKeysBRect.x1, selectedKeysBRect.y2);
    QPointF bottomRight = zoomContext.toWidgetCoordinates(selectedKeysBRect.x2, selectedKeysBRect.y1);
    
    return (widgetPos.x() >= (topLeft.x() - DISTANCE_ACCEPTANCE_FROM_READER_EDGE) &&
            widgetPos.x() <= (topLeft.x()) &&
            widgetPos.y() <= (bottomRight.y() + DISTANCE_ACCEPTANCE_FROM_READER_EDGE) &&
            widgetPos.y() >= (topLeft.y() - DISTANCE_ACCEPTANCE_FROM_READER_EDGE));
}

bool
DopeSheetViewPrivate::isNearbySelectedKeysBRec(const QPointF& widgetPos) const
{
    QPointF topLeft = zoomContext.toWidgetCoordinates(selectedKeysBRect.x1, selectedKeysBRect.y2);
    QPointF bottomRight = zoomContext.toWidgetCoordinates(selectedKeysBRect.x2, selectedKeysBRect.y1);
    
    return (widgetPos.x() >= (topLeft.x()) &&
            widgetPos.x() <= (bottomRight.x()) &&
            widgetPos.y() <= (bottomRight.y() + DISTANCE_ACCEPTANCE_FROM_READER_EDGE) &&
            widgetPos.y() >= (topLeft.y() - DISTANCE_ACCEPTANCE_FROM_READER_EDGE));

}

std::vector<DopeSheetKey> DopeSheetViewPrivate::isNearByKeyframe(const boost::shared_ptr<DSKnob> &dsKnob, const QPointF &widgetCoords) const
{
    assert(dsKnob);

    std::vector<DopeSheetKey> ret;

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
                boost::shared_ptr<DSKnob> context;
                if (dim == -1) {
                    QTreeWidgetItem *childItem = dsKnob->findDimTreeItem(i);
                    context = model->mapNameItemToDSKnob(childItem);
                }
                else {
                    context = dsKnob;
                }

                ret.push_back(DopeSheetKey(context, kf));
            }
        }
    }

    return ret;
}

std::vector<DopeSheetKey> DopeSheetViewPrivate::isNearByKeyframe(boost::shared_ptr<DSNode> dsNode, const QPointF &widgetCoords) const
{
    std::vector<DopeSheetKey> ret;

    const DSTreeItemKnobMap& dsKnobs = dsNode->getItemKnobMap();

    for (DSTreeItemKnobMap::const_iterator it = dsKnobs.begin(); it != dsKnobs.end(); ++it) {
        boost::shared_ptr<DSKnob> dsKnob = (*it).second;
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
                ret.push_back(DopeSheetKey(dsKnob, kf));
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
    QImage kfTexturesImages[KF_TEXTURES_COUNT];
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
    
    glGenTextures(KF_TEXTURES_COUNT, kfTexturesIDs);
    
    glEnable(GL_TEXTURE_2D);
    
    for (int i = 0; i < KF_TEXTURES_COUNT; ++i) {
        if (std::max(kfTexturesImages[i].width(), kfTexturesImages[i].height()) != KF_PIXMAP_SIZE) {
            kfTexturesImages[i] = kfTexturesImages[i].scaled(KF_PIXMAP_SIZE, KF_PIXMAP_SIZE, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        }
        kfTexturesImages[i] = QGLWidget::convertToGLFormat(kfTexturesImages[i]);
        glBindTexture(GL_TEXTURE_2D, kfTexturesIDs[i]);
        
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, KF_PIXMAP_SIZE, KF_PIXMAP_SIZE, 0, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8_REV, kfTexturesImages[i].bits());

    }
    
    glBindTexture(GL_TEXTURE_2D, 0);
    glDisable(GL_TEXTURE_2D);
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
        ret = (selected) ? DopeSheetViewPrivate::kfTextureInterpCurveSelected : DopeSheetViewPrivate::kfTextureInterpCurve;
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
    scaleColor.setRgbF(Natron::clamp(scaleR, 0., 1.),
                       Natron::clamp(scaleG, 0., 1.),
                       Natron::clamp(scaleB, 0., 1.));

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

        glCheckError();

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

            glCheckErrorIgnoreOSXBug();

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

    DSTreeItemNodeMap treeItemsAndDSNodes = model->getItemNodeMap();

    // Perform drawing
    {
        GLProtectAttrib a(GL_CURRENT_BIT | GL_COLOR_BUFFER_BIT | GL_ENABLE_BIT);

        for (DSTreeItemNodeMap::const_iterator it = treeItemsAndDSNodes.begin();
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

            boost::shared_ptr<DSNode> dsNode = (*it).second;

            drawNodeRow(dsNode);

            const DSTreeItemKnobMap& knobItems = dsNode->getItemKnobMap();
            for (DSTreeItemKnobMap::const_iterator it2 = knobItems.begin();
                 it2 != knobItems.end();
                 ++it2) {

                boost::shared_ptr<DSKnob> dsKnob = (*it2).second;

                drawKnobRow(dsKnob);
            }

            if (boost::shared_ptr<DSNode> group = model->getGroupDSNode(dsNode.get())) {
                drawGroupOverlay(dsNode, group);
            }

            Natron::DopeSheetItemType nodeType = dsNode->getItemType();

            if (dsNode->isRangeDrawingEnabled()) {
                drawRange(dsNode);
            }

            if (nodeType != Natron::eDopeSheetItemTypeGroup) {
                drawKeyframes(dsNode);
            }
        }

        // Draw node rows separations
        for (DSTreeItemNodeMap::const_iterator it = treeItemsAndDSNodes.begin();
             it != treeItemsAndDSNodes.end();
             ++it) {
            boost::shared_ptr<DSNode> dsNode = (*it).second;
            bool isTreeViewTopItem = !hierarchyView->itemAbove(dsNode->getTreeItem());
            if (!isTreeViewTopItem) {
                drawNodeRowSeparation(dsNode);
            }
        }
    }
}

/**
 * @brief DopeSheetViewPrivate::drawNodeRow
 *
 *
 */
void DopeSheetViewPrivate::drawNodeRow(const boost::shared_ptr<DSNode> dsNode) const
{
    GLProtectAttrib a(GL_CURRENT_BIT | GL_COLOR_BUFFER_BIT | GL_ENABLE_BIT);

    QRectF nameItemRect = hierarchyView->visualItemRect(dsNode->getTreeItem());
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
void DopeSheetViewPrivate::drawKnobRow(const boost::shared_ptr<DSKnob> dsKnob) const
{
    if (dsKnob->getTreeItem()->isHidden()) {
        return;
    }

    GLProtectAttrib a(GL_CURRENT_BIT | GL_COLOR_BUFFER_BIT | GL_ENABLE_BIT);

    QRectF nameItemRect = hierarchyView->visualItemRect(dsKnob->getTreeItem());
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

void DopeSheetViewPrivate::drawNodeRowSeparation(const boost::shared_ptr<DSNode> dsNode) const
{
    GLProtectAttrib a(GL_CURRENT_BIT | GL_COLOR_BUFFER_BIT | GL_ENABLE_BIT | GL_LINE_BIT);

    QRectF nameItemRect = hierarchyView->visualItemRect(dsNode->getTreeItem());
    QRectF rowRect = nameItemRectToRowRect(nameItemRect);

    glLineWidth(appPTR->getCurrentSettings()->getDopeSheetEditorNodeSeparationWith());
    glColor4f(0.f, 0.f, 0.f, 1.f);

    glBegin(GL_LINES);
    glVertex2f(rowRect.left(), rowRect.top());
    glVertex2f(rowRect.right(), rowRect.top());
    glEnd();
}

void DopeSheetViewPrivate::drawRange(const boost::shared_ptr<DSNode> &dsNode) const
{
    // Draw the clip
    {
        std::map<DSNode *, FrameRange >::const_iterator foundRange = nodeRanges.find(dsNode.get());
        if (foundRange == nodeRanges.end()) {
            return;
        }
        

        
        const FrameRange& range = foundRange->second;
        QRectF treeItemRect = hierarchyView->visualItemRect(dsNode->getTreeItem());

        QPointF treeRectTopLeft = treeItemRect.topLeft();
        treeRectTopLeft = zoomContext.toZoomCoordinates(treeRectTopLeft.x(), treeRectTopLeft.y());
        QPointF treeRectBtmRight = treeItemRect.bottomRight();
        treeRectBtmRight = zoomContext.toZoomCoordinates(treeRectBtmRight.x(), treeRectBtmRight.y());
        
        RectD clipRectZoomCoords;
        clipRectZoomCoords.x1 = range.first;
        clipRectZoomCoords.x2 = range.second;
        clipRectZoomCoords.y2 = treeRectTopLeft.y();
        clipRectZoomCoords.y1 = treeRectBtmRight.y();
        GLProtectAttrib a(GL_CURRENT_BIT);

        QColor fillColor = dsNode->getNodeGui()->getCurrentColor();
        fillColor = QColor::fromHsl(fillColor.hslHue(), 50, fillColor.lightness());

        bool isSelected = model->getSelectionModel()->rangeIsSelected(dsNode);

        
        // If necessary, draw the original frame range line
        float clipRectCenterY = 0.;
        if (isSelected && dsNode->getItemType() == Natron::eDopeSheetItemTypeReader) {
            NodePtr node = dsNode->getInternalNode();

            Knob<int> *firstFrameKnob = dynamic_cast<Knob<int> *>(node->getKnobByName(kReaderParamNameFirstFrame).get());
            assert(firstFrameKnob);

            double speedValue = 1.0f;

            Knob<int> *originalFrameRangeKnob = dynamic_cast<Knob<int> *>(node->getKnobByName(kReaderParamNameOriginalFrameRange).get());
            assert(originalFrameRangeKnob);

            int originalFirstFrame = originalFrameRangeKnob->getValue(0);
            int originalLastFrame = originalFrameRangeKnob->getValue(1);
            int firstFrame = firstFrameKnob->getValue();
            int lineBegin = range.first - (firstFrame - originalFirstFrame);

            int frameCount = originalLastFrame - originalFirstFrame + 1;
            int lineEnd = lineBegin + (frameCount / speedValue);

            clipRectCenterY = (clipRectZoomCoords.y1 + clipRectZoomCoords.y2) / 2.;

            GLProtectAttrib aa(GL_CURRENT_BIT | GL_LINE_BIT);
            glLineWidth(2);
            
            glColor4f(fillColor.redF(), fillColor.greenF(), fillColor.blueF(), 1.f);
            
            glBegin(GL_LINES);
            
            //horizontal line
            glVertex2f(lineBegin, clipRectCenterY);
            glVertex2f(lineEnd, clipRectCenterY);
            
            //left end
            glVertex2d(lineBegin, clipRectZoomCoords.y1);
            glVertex2d(lineBegin, clipRectZoomCoords.y2);
            
            
            //right end
            glVertex2d(lineEnd, clipRectZoomCoords.y1);
            glVertex2d(lineEnd, clipRectZoomCoords.y2);
            
            glEnd();

        }
        
        QColor fadedColor;
        fadedColor.setRgbF(fillColor.redF() * 0.5, fillColor.greenF() * 0.5, fillColor.blueF() * 0.5);
        // Fill the range rect
        

        glColor4f(fadedColor.redF(), fadedColor.greenF(), fadedColor.blueF(), 1.f);
        
        glBegin(GL_POLYGON);
        glVertex2f(clipRectZoomCoords.left(), clipRectZoomCoords.top());
        glVertex2f(clipRectZoomCoords.left(), clipRectZoomCoords.bottom());
        glVertex2f(clipRectZoomCoords.right(), clipRectZoomCoords.bottom());
        glVertex2f(clipRectZoomCoords.right(), clipRectZoomCoords.top());
        glEnd();
        
        if (isSelected) {
            glColor4f(fillColor.redF(), fillColor.greenF(), fillColor.blueF(), 1.f);
            glBegin(GL_LINE_LOOP);
            glVertex2f(clipRectZoomCoords.left(), clipRectZoomCoords.top());
            glVertex2f(clipRectZoomCoords.left(), clipRectZoomCoords.bottom());
            glVertex2f(clipRectZoomCoords.right(), clipRectZoomCoords.bottom());
            glVertex2f(clipRectZoomCoords.right(), clipRectZoomCoords.top());
            glEnd();
        }
        
        if (isSelected && dsNode->getItemType() == Natron::eDopeSheetItemTypeReader) {
            boost::shared_ptr<Settings> settings = appPTR->getCurrentSettings();
            double selectionColorRGB[3];
            settings->getSelectionColor(&selectionColorRGB[0], &selectionColorRGB[1], &selectionColorRGB[2]);
            QColor selectionColor;
            selectionColor.setRgbF(selectionColorRGB[0], selectionColorRGB[1], selectionColorRGB[2]);
            
            QFontMetrics fm(*font);
            int fontHeigt = fm.height();
            
            QString leftText = QString::number(range.first);
            QString rightText = QString::number(range.second - 1);
            
            int rightTextW = fm.width(rightText);
            QPointF textLeftPos(zoomContext.toZoomCoordinates(zoomContext.toWidgetCoordinates(range.first, 0).x() + 3,0).x(),
                                zoomContext.toZoomCoordinates(0,zoomContext.toWidgetCoordinates(0,clipRectCenterY).y() + fontHeigt / 2.).y());
            
            renderText(textLeftPos.x(), textLeftPos.y(), leftText, selectionColor, *font);
            
            QPointF textRightPos(zoomContext.toZoomCoordinates(zoomContext.toWidgetCoordinates(range.second, 0).x() - rightTextW - 3,0).x(),
                                 zoomContext.toZoomCoordinates(0,zoomContext.toWidgetCoordinates(0,clipRectCenterY).y() + fontHeigt / 2.).y());
            
            renderText(textRightPos.x(), textRightPos.y(), rightText, selectionColor, *font);
        }
    }
}


/**
 * @brief DopeSheetViewPrivate::drawKeyframes
 *
 *
 */
void DopeSheetViewPrivate::drawKeyframes(const boost::shared_ptr<DSNode> &dsNode) const
{
    running_in_main_thread_and_context(q_ptr);

    boost::shared_ptr<Settings> settings = appPTR->getCurrentSettings();
    double selectionColorRGB[3];
    settings->getSelectionColor(&selectionColorRGB[0], &selectionColorRGB[1], &selectionColorRGB[2]);
    QColor selectionColor;
    selectionColor.setRgbF(selectionColorRGB[0], selectionColorRGB[1], selectionColorRGB[2]);

    // Perform drawing
    {
        GLProtectAttrib a(GL_CURRENT_BIT | GL_COLOR_BUFFER_BIT | GL_ENABLE_BIT);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        const DSTreeItemKnobMap& knobItems = dsNode->getItemKnobMap();
        
        int kfTimeSelected;
        int hasSingleKfTimeSelected = model->getSelectionModel()->hasSingleKeyFrameTimeSelected(&kfTimeSelected);

        std::map<double, bool> nodeKeytimes;
        std::map<DSKnob *, std::map<double, bool> > knobsKeytimes;

        for (DSTreeItemKnobMap::const_iterator it = knobItems.begin();
             it != knobItems.end();
             ++it) {
            boost::shared_ptr<DSKnob> dsKnob = (*it).second;
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

                // Clip keyframes horizontally //TODO Clip vertically too
                if (keyTime < zoomContext.left() || keyTime > zoomContext.right()) {
                    continue;
                }

                double rowCenterYWidget = hierarchyView->visualItemRect(dsKnob->getTreeItem()).center().y();
                RectD zoomKfRect = getKeyFrameBoundingRectZoomCoords(keyTime, rowCenterYWidget);
                bool kfSelected = model->getSelectionModel()->keyframeIsSelected(dsKnob, kf);

                // Draw keyframe in the knob dim row only if it's visible
                bool drawInDimRow = hierarchyView->itemIsVisibleFromOutside(knobTreeItem);

                if (drawInDimRow) {
                    DopeSheetViewPrivate::KeyframeTexture texType = kfTextureFromKeyframeType(kf.getInterpolation(),
                                                                                              kfSelected || selectionRect.intersects(zoomKfRect));

                    if (texType != DopeSheetViewPrivate::kfTextureNone) {
                        drawTexturedKeyframe(texType, hasSingleKfTimeSelected && kfSelected,
                                             kfTimeSelected, selectionColor,zoomKfRect);
                    }
                }

                // Fill the knob times map
                if (boost::shared_ptr<DSKnob> rootDSKnob = model->mapNameItemToDSKnob(knobTreeItem->parent())) {
                    assert(rootDSKnob);
                    const std::map<double, bool>& map = knobsKeytimes[rootDSKnob.get()];
                    bool knobTimeExists = (map.find(keyTime) != map.end());

                    if (!knobTimeExists) {
                        knobsKeytimes[rootDSKnob.get()][keyTime] = kfSelected;
                    }
                    else {
                        bool knobTimeIsSelected = knobsKeytimes[rootDSKnob.get()][keyTime];

                        if (!knobTimeIsSelected && kfSelected) {
                            knobsKeytimes[rootDSKnob.get()][keyTime] = true;
                        }
                    }
                }

                // Fill the node times map
                bool nodeTimeExists = (nodeKeytimes.find(keyTime) != nodeKeytimes.end());

                if (!nodeTimeExists) {
                    nodeKeytimes[keyTime] = kfSelected;
                }
                else {
                    bool nodeTimeIsSelected = nodeKeytimes[keyTime];

                    if (!nodeTimeIsSelected && kfSelected) {
                        nodeKeytimes[keyTime] = true;
                    }
                }
            }
        }

        // Draw master keys in knob root section
        for (std::map<DSKnob *, std::map<double, bool> >::const_iterator it = knobsKeytimes.begin();
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

            QTreeWidgetItem *nodeItem = dsNode->getTreeItem();

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
}

void DopeSheetViewPrivate::drawTexturedKeyframe(DopeSheetViewPrivate::KeyframeTexture textureType,
                                                bool drawTime,
                                                int time,
                                                const QColor& textColor,
                                                const RectD &rect) const
{
    GLProtectAttrib a(GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT | GL_CURRENT_BIT | GL_TRANSFORM_BIT);
    GLProtectMatrix pr(GL_MODELVIEW);

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, kfTexturesIDs[textureType]);

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
    
    if (drawTime) {
        QString text = QString::number(time);
        QPointF p = zoomContext.toWidgetCoordinates(rect.right(), rect.bottom());
        p.rx() += 3;
        p = zoomContext.toZoomCoordinates(p.x(), p.y());
        renderText(p.x(), p.y(), text, textColor, *font);
    }
}

void DopeSheetViewPrivate::drawGroupOverlay(const boost::shared_ptr<DSNode> &dsNode, const boost::shared_ptr<DSNode> &group) const
{
    // Get the overlay color
    double r, g, b;
    dsNode->getNodeGui()->getColor(&r, &g, &b);

    // Compute the area to fill
    int height = hierarchyView->getHeightForItemAndChildren(dsNode->getTreeItem()) ;
    QRectF nameItemRect = hierarchyView->visualItemRect(dsNode->getTreeItem());

    FrameRange groupRange = nodeRanges.at(group.get());

    RectD overlayRect;
    overlayRect.x1 = groupRange.first;
    overlayRect.x2 = groupRange.second;
    
    overlayRect.y1 = zoomContext.toZoomCoordinates(0, nameItemRect.top() + height).y();
    overlayRect.y2 = zoomContext.toZoomCoordinates(0, nameItemRect.top()).y();

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

    double projectStart, projectEnd;
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
        glVertex2f(selectionRect.x1, selectionRect.y1);
        glVertex2f(selectionRect.x1, selectionRect.y2);
        glVertex2f(selectionRect.x2, selectionRect.y2);
        glVertex2f(selectionRect.x2, selectionRect.y1);
        glEnd();

        glLineWidth(1.5);

        // Draw outline
        glColor4f(0.5,0.5,0.5,1.);
        glBegin(GL_LINE_LOOP);
        glVertex2f(selectionRect.x1, selectionRect.y1);
        glVertex2f(selectionRect.x1, selectionRect.y2);
        glVertex2f(selectionRect.x2, selectionRect.y2);
        glVertex2f(selectionRect.x2, selectionRect.y1);
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
        glVertex2f(selectedKeysBRect.left(), selectedKeysBRect.bottom());
        glVertex2f(selectedKeysBRect.left(), selectedKeysBRect.top());
        glVertex2f(selectedKeysBRect.right(), selectedKeysBRect.top());
        glVertex2f(selectedKeysBRect.right(), selectedKeysBRect.bottom());
        glEnd();

        // Draw center cross lines
        const int CROSS_LINE_OFFSET = 10;

        QPointF bRectCenter((selectedKeysBRect.x1 + selectedKeysBRect.x2) / 2., (selectedKeysBRect.y1 + selectedKeysBRect.y2) / 2.);
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
    selectionRect.x1 = std::min(origin.x(), current.x());
    selectionRect.x2 = std::max(origin.x(), current.x());
    selectionRect.y1 = std::min(origin.y(), current.y());
    selectionRect.y2 = std::max(origin.y(), current.y());
}

void DopeSheetViewPrivate::computeSelectedKeysBRect()
{
    DSKeyPtrList selectedKeyframes;
    std::vector<boost::shared_ptr<DSNode> > selectedNodes;
    model->getSelectionModel()->getCurrentSelection(&selectedKeyframes, &selectedNodes);

    if ((selectedKeyframes.size() <= 1 && selectedNodes.empty()) ||
            (selectedKeyframes.empty() && selectedNodes.size() <= 1)) {
        selectedKeysBRect.clear();
        return;
    }

    selectedKeysBRect.setupInfinity();
    
    for (DSKeyPtrList::const_iterator it = selectedKeyframes.begin();
         it != selectedKeyframes.end();
         ++it) {
        boost::shared_ptr<DSKnob> knobContext = (*it)->context.lock();
        assert(knobContext);

        QTreeWidgetItem *keyItem = knobContext->getTreeItem();

        double time = (*it)->key.getTime();
        
        //x1,x2 are in zoom coords
        selectedKeysBRect.x1 = std::min(time,selectedKeysBRect.x1);
        selectedKeysBRect.x2 = std::max(time,selectedKeysBRect.x2);
        
        
        QRect keyItemRect = hierarchyView->visualItemRect(keyItem);
        if (!keyItemRect.isNull() && !keyItemRect.isEmpty()) {
            double y = keyItemRect.center().y();
            
            //y1,y2 are in widget coords
            selectedKeysBRect.y1 = std::min(y,selectedKeysBRect.y1);
            selectedKeysBRect.y2 = std::max(y,selectedKeysBRect.y2);
        }

    }

    for (std::vector<boost::shared_ptr<DSNode> >::iterator it = selectedNodes.begin(); it!=selectedNodes.end(); ++it) {
        std::map<DSNode *, FrameRange >::const_iterator foundRange = nodeRanges.find(it->get());
        if (foundRange == nodeRanges.end()) {
            continue;
        }
        const FrameRange& range = foundRange->second;
        
        //x1,x2 are in zoom coords
        selectedKeysBRect.x1 = std::min(range.first, selectedKeysBRect.x1);
        selectedKeysBRect.x2 = std::max(range.second, selectedKeysBRect.x2);
        
        QTreeWidgetItem* nodeItem = (*it)->getTreeItem();
        QRect itemRect = hierarchyView->visualItemRect(nodeItem);
        if (!itemRect.isNull() && !itemRect.isEmpty()) {
            double y = itemRect.center().y();
            
            //y1,y2 are in widget coords
            selectedKeysBRect.y1 = std::min(y,selectedKeysBRect.y1);
            selectedKeysBRect.y2 = std::max(y,selectedKeysBRect.y2);
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

    if (!selectedKeysBRect.isNull()) {
        
        /// Adjust the bounding rect of the size of a keyframe
        double leftWidget = zoomContext.toWidgetCoordinates(selectedKeysBRect.x1, 0).x();
        double leftAdjustedZoom = zoomContext.toZoomCoordinates(leftWidget - KF_X_OFFSET, 0).x();
        double xAdjustOffset = (selectedKeysBRect.x1 - leftAdjustedZoom);

        selectedKeysBRect.x1 -= xAdjustOffset;
        selectedKeysBRect.x2 += xAdjustOffset;
    }
}

void DopeSheetViewPrivate::computeRangesBelow(DSNode *dsNode)
{
    DSTreeItemNodeMap nodeRows = model->getItemNodeMap();

    for (DSTreeItemNodeMap::const_iterator it = nodeRows.begin(); it != nodeRows.end(); ++it) {
        QTreeWidgetItem *item = (*it).first;
        boost::shared_ptr<DSNode> toCompute = (*it).second;

        if (hierarchyView->visualItemRect(item).y() >= hierarchyView->visualItemRect(dsNode->getTreeItem()).y()) {
            computeNodeRange(toCompute.get());
        }
    }
}

void DopeSheetViewPrivate::computeNodeRange(DSNode *dsNode)
{
    Natron::DopeSheetItemType nodeType = dsNode->getItemType();

    switch (nodeType) {
    case Natron::eDopeSheetItemTypeReader:
        computeReaderRange(dsNode);
        break;
    case Natron::eDopeSheetItemTypeRetime:
        computeRetimeRange(dsNode);
        break;
    case Natron::eDopeSheetItemTypeTimeOffset:
        computeTimeOffsetRange(dsNode);
        break;
    case Natron::eDopeSheetItemTypeFrameRange:
        computeFRRange(dsNode);
        break;
    case Natron::eDopeSheetItemTypeGroup:
        computeGroupRange(dsNode);
        break;
    default:
        break;
    }
}

void DopeSheetViewPrivate::computeReaderRange(DSNode *reader)
{
    NodePtr node = reader->getInternalNode();

    Knob<int> *startingTimeKnob = dynamic_cast<Knob<int> *>(node->getKnobByName(kReaderParamNameStartingTime).get());
    assert(startingTimeKnob);
    Knob<int> *firstFrameKnob = dynamic_cast<Knob<int> *>(node->getKnobByName(kReaderParamNameFirstFrame).get());
    assert(firstFrameKnob);
    Knob<int> *lastFrameKnob = dynamic_cast<Knob<int> *>(node->getKnobByName(kReaderParamNameLastFrame).get());
    assert(lastFrameKnob);

    int startingTimeValue = startingTimeKnob->getValue();
    int firstFrameValue = firstFrameKnob->getValue();
    int lastFrameValue = lastFrameKnob->getValue();

    FrameRange range(startingTimeValue,
                     startingTimeValue + (lastFrameValue - firstFrameValue) + 1);

    nodeRanges[reader] = range;

    if (boost::shared_ptr<DSNode> isInGroup = model->getGroupDSNode(reader)) {
        computeGroupRange(isInGroup.get());
    }

    if (boost::shared_ptr<DSNode> isConnectedToTimeNode = model->getNearestTimeNodeFromOutputs(reader)) {
        computeNodeRange(isConnectedToTimeNode.get());
    }
}

void DopeSheetViewPrivate::computeRetimeRange(DSNode *retimer)
{
    NodePtr node = retimer->getInternalNode();
    NodePtr input = node->getInput(0);
    if (input) {
        
        U64 nodeHash = node->getHashValue();
        double inputFirst,inputLast;
        input->getLiveInstance()->getFrameRange_public(input->getHashValue(), &inputFirst, &inputLast);

        FramesNeededMap framesFirst = node->getLiveInstance()->getFramesNeeded_public(nodeHash, inputFirst, 0, 0);
        FramesNeededMap framesLast = node->getLiveInstance()->getFramesNeeded_public(nodeHash, inputLast, 0, 0);
        assert(!framesFirst.empty() && !framesLast.empty());
        
        FrameRange range;
        {
            std::map<int, std::vector<OfxRangeD> >& rangeFirst = framesFirst[0];
            assert(!rangeFirst.empty());
            std::vector<OfxRangeD>& frames = rangeFirst[0];
            assert(!frames.empty());
            range.first = (frames.front().min);
        }
        {
            std::map<int, std::vector<OfxRangeD> >& rangeLast = framesLast[0];
            assert(!rangeLast.empty());
            std::vector<OfxRangeD>& frames = rangeLast[0];
            assert(!frames.empty());
            range.second = (frames.front().min);
        }

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
    if (boost::shared_ptr<DSNode> nearestReader = model->findDSNode(model->getNearestReader(timeOffset))) {
        FrameRange nearestReaderRange = nodeRanges.at(nearestReader.get());

        // Retrieve the time offset values
        Knob<int> *timeOffsetKnob = dynamic_cast<Knob<int> *>(timeOffset->getInternalNode()->getKnobByName(kReaderParamNameTimeOffset).get());
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
    assert(node);
    if (!node) {
        throw std::logic_error("DopeSheetViewPrivate::computeGroupRange: node is NULL");
    }

    FrameRange range;
    std::set<double> times;

    NodeGroup* nodegroup = dynamic_cast<NodeGroup *>(node->getLiveInstance());
    assert(nodegroup);
    if (!nodegroup) {
        throw std::logic_error("DopeSheetViewPrivate::computeGroupRange: node is not a group");
    }
    NodeList nodes = nodegroup->getNodes();

    for (NodeList::const_iterator it = nodes.begin(); it != nodes.end(); ++it) {
        NodePtr node = (*it);

        boost::shared_ptr<DSNode> dsNode = model->findDSNode(node.get());
        
        if (!dsNode) {
            continue;
        }

        boost::shared_ptr<NodeGui> nodeGui = boost::dynamic_pointer_cast<NodeGui>(node->getNodeGui());

        if (!nodeGui->getSettingPanel() || !nodeGui->isSettingsPanelVisible()) {
            continue;
        }
        
        computeNodeRange(dsNode.get());
        
        std::map<DSNode *, FrameRange >::iterator found = nodeRanges.find(dsNode.get());
        if (found != nodeRanges.end()) {
            times.insert(found->second.first);
            times.insert(found->second.second);
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

void DopeSheetViewPrivate::onMouseLeftButtonDrag(QMouseEvent *e)
{
    QPointF mouseZoomCoords = zoomContext.toZoomCoordinates(e->x(), e->y());
    QPointF lastZoomCoordsOnMousePress = zoomContext.toZoomCoordinates(lastPosOnMousePress.x(),
                                                                       lastPosOnMousePress.y());
    QPointF lastZoomCoordsOnMouseMove = zoomContext.toZoomCoordinates(lastPosOnMouseMove.x(),
                                                                      lastPosOnMouseMove.y());

    
    double currentTime = mouseZoomCoords.x();

    double dt = clampedMouseOffset(lastZoomCoordsOnMousePress.x(), currentTime);
    double dv = mouseZoomCoords.y() - lastZoomCoordsOnMouseMove.y();
    
    switch (eventState) {
    case DopeSheetView::esMoveKeyframeSelection:
    {
        if (dt >= 1.0f || dt <= -1.0f) {
            model->moveSelectedKeysAndNodes(dt);
        }

        break;
    }
    case DopeSheetView::esTransformingKeyframesMiddleLeft:
    case DopeSheetView::esTransformingKeyframesMiddleRight:
    {
        bool shiftHeld = modCASIsShift(e);
        QPointF center;
        if (!shiftHeld) {
            if (eventState == DopeSheetView::esTransformingKeyframesMiddleLeft) {
                center.rx() = selectedKeysBRect.x2;
            } else {
                center.rx() = selectedKeysBRect.x1;
            }
            center.ry() = (selectedKeysBRect.y1 + selectedKeysBRect.y2)/2.;
        } else {
            center = QPointF((selectedKeysBRect.x1 + selectedKeysBRect.x2)/2.,(selectedKeysBRect.y1 + selectedKeysBRect.y2)/2.);
        }
        
        
        double sx = 1.,sy = 1.;
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
        
        Transform::Matrix3x3 transform = Transform::matTransformCanonical(tx, ty, sx, sy, 0, 0, true, 0, center.x(), center.y());
        model->transformSelectedKeys(transform);
    }   break;
    case DopeSheetView::esMoveCurrentFrameIndicator:
    {
        if (dt >= 1.0f || dt <= -1.0f) {
            moveCurrentFrameIndicator(dt);
        }

        break;
    }
    case DopeSheetView::esSelectionByRect:
    {
        computeSelectionRect(lastZoomCoordsOnMousePress, mouseZoomCoords);

        q_ptr->redraw();

        break;
    }
    case DopeSheetView::esReaderLeftTrim:
    {
        if (dt >= 1.0f || dt <= -1.0f) {
            Knob<int> *timeOffsetKnob = dynamic_cast<Knob<int> *>(currentEditedReader->getInternalNode()->getKnobByName(kReaderParamNameTimeOffset).get());
            assert(timeOffsetKnob);

            double newFirstFrame = std::floor(currentTime - timeOffsetKnob->getValue() + 0.5);

            model->trimReaderLeft(currentEditedReader, newFirstFrame);
        }

        break;
    }
    case DopeSheetView::esReaderRightTrim:
    {
        if (dt >= 1.0f || dt <= -1.0f) {
            Knob<int> *timeOffsetKnob = dynamic_cast<Knob<int> *>(currentEditedReader->getInternalNode()->getKnobByName(kReaderParamNameTimeOffset).get());
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
    case DopeSheetView::esNoEditingState:
        eventState = DopeSheetView::esSelectionByRect;

        break;
    default:
        break;
    }
}

void DopeSheetViewPrivate::createSelectionFromRect(const RectD &zoomCoordsRect, std::vector<DopeSheetKey> *result, std::vector<boost::shared_ptr<DSNode> >* selectedNodes)
{
    DSTreeItemNodeMap dsNodes = model->getItemNodeMap();

    for (DSTreeItemNodeMap::const_iterator it = dsNodes.begin(); it != dsNodes.end(); ++it) {
        const boost::shared_ptr<DSNode>& dsNode = (*it).second;

        const DSTreeItemKnobMap& dsKnobs = dsNode->getItemKnobMap();

        for (DSTreeItemKnobMap::const_iterator it2 = dsKnobs.begin(); it2 != dsKnobs.end(); ++it2) {
            boost::shared_ptr<DSKnob> dsKnob = (*it2).second;
            int dim = dsKnob->getDimension();

            if (dim == -1) {
                continue;
            }

            KeyFrameSet keyframes = dsKnob->getKnobGui()->getCurve(dim)->getKeyFrames_mt_safe();

            for (KeyFrameSet::const_iterator kIt = keyframes.begin();
                 kIt != keyframes.end();
                 ++kIt) {
                KeyFrame kf = (*kIt);

                double y = hierarchyView->visualItemRect(dsKnob->getTreeItem()).center().y();

                RectD zoomKfRect = getKeyFrameBoundingRectZoomCoords(kf.getTime(), y);

                if (zoomCoordsRect.intersects(zoomKfRect)) {
                    result->push_back(DopeSheetKey(dsKnob, kf));
                }
            }
        }
        
        std::map<DSNode *, FrameRange >::const_iterator foundRange = nodeRanges.find(dsNode.get());
        if (foundRange != nodeRanges.end()) {
            QPoint visualRectCenter = hierarchyView->visualItemRect(dsNode->getTreeItem()).center();
            QPointF center = zoomContext.toZoomCoordinates(visualRectCenter.x(),visualRectCenter.y());
            if (zoomCoordsRect.contains((foundRange->second.first + foundRange->second.second) / 2. , center.y())) {
                selectedNodes->push_back(dsNode);
            }
        }
    }
}

void DopeSheetViewPrivate::moveCurrentFrameIndicator(double dt)
{
    gui->getApp()->setLastViewerUsingTimeline(boost::shared_ptr<Natron::Node>());

    double toTime = timeline->currentFrame() + dt;
    
    gui->setDraftRenderEnabled(true);
    timeline->seekFrame(SequenceTime(toTime), false, 0,
                        Natron::eTimelineChangeReasonDopeSheetEditorSeek);
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
                     q_ptr, SLOT(onSelectedAllTriggered()));
    editMenu->addAction(selectAllKeyframesAction);


    // Interpolation actions
    QAction *constantInterpAction = new ActionWithShortcut(kShortcutGroupDopeSheetEditor,
                                                           kShortcutIDActionCurveEditorConstant,
                                                           kShortcutDescActionCurveEditorConstant,
                                                           interpMenu);
    QPixmap pix;
    appPTR->getIcon(Natron::NATRON_PIXMAP_INTERP_CONSTANT, NATRON_MEDIUM_BUTTON_ICON_SIZE, &pix);
    constantInterpAction->setIcon(QIcon(pix));
    constantInterpAction->setIconVisibleInMenu(true);

    QObject::connect(constantInterpAction, SIGNAL(triggered()),
                     q_ptr, SLOT(constantInterpSelectedKeyframes()));

    interpMenu->addAction(constantInterpAction);

    QAction *linearInterpAction = new ActionWithShortcut(kShortcutGroupDopeSheetEditor,
                                                         kShortcutIDActionCurveEditorLinear,
                                                         kShortcutDescActionCurveEditorLinear,
                                                         interpMenu);

    appPTR->getIcon(Natron::NATRON_PIXMAP_INTERP_LINEAR, NATRON_MEDIUM_BUTTON_ICON_SIZE, &pix);
    linearInterpAction->setIcon(QIcon(pix));
    linearInterpAction->setIconVisibleInMenu(true);

    QObject::connect(linearInterpAction, SIGNAL(triggered()),
                     q_ptr, SLOT(linearInterpSelectedKeyframes()));

    interpMenu->addAction(linearInterpAction);

    QAction *smoothInterpAction = new ActionWithShortcut(kShortcutGroupDopeSheetEditor,
                                                         kShortcutIDActionCurveEditorSmooth,
                                                         kShortcutDescActionCurveEditorSmooth,
                                                         interpMenu);

    appPTR->getIcon(Natron::NATRON_PIXMAP_INTERP_CURVE_Z, NATRON_MEDIUM_BUTTON_ICON_SIZE, &pix);
    smoothInterpAction->setIcon(QIcon(pix));
    smoothInterpAction->setIconVisibleInMenu(true);

    QObject::connect(smoothInterpAction, SIGNAL(triggered()),
                     q_ptr, SLOT(smoothInterpSelectedKeyframes()));

    interpMenu->addAction(smoothInterpAction);

    QAction *catmullRomInterpAction = new ActionWithShortcut(kShortcutGroupDopeSheetEditor,
                                                             kShortcutIDActionCurveEditorCatmullrom,
                                                             kShortcutDescActionCurveEditorCatmullrom,
                                                             interpMenu);
    appPTR->getIcon(Natron::NATRON_PIXMAP_INTERP_CURVE_R, NATRON_MEDIUM_BUTTON_ICON_SIZE, &pix);
    catmullRomInterpAction->setIcon(QIcon(pix));
    catmullRomInterpAction->setIconVisibleInMenu(true);

    QObject::connect(catmullRomInterpAction, SIGNAL(triggered()),
                     q_ptr, SLOT(catmullRomInterpSelectedKeyframes()));

    interpMenu->addAction(catmullRomInterpAction);

    QAction *cubicInterpAction = new ActionWithShortcut(kShortcutGroupDopeSheetEditor,
                                                        kShortcutIDActionCurveEditorCubic,
                                                        kShortcutDescActionCurveEditorCubic,
                                                        interpMenu);
    appPTR->getIcon(Natron::NATRON_PIXMAP_INTERP_CURVE_C, NATRON_MEDIUM_BUTTON_ICON_SIZE, &pix);
    cubicInterpAction->setIcon(QIcon(pix));
    cubicInterpAction->setIconVisibleInMenu(true);

    QObject::connect(cubicInterpAction, SIGNAL(triggered()),
                     q_ptr, SLOT(cubicInterpSelectedKeyframes()));

    interpMenu->addAction(cubicInterpAction);

    QAction *horizontalInterpAction = new ActionWithShortcut(kShortcutGroupDopeSheetEditor,
                                                             kShortcutIDActionCurveEditorHorizontal,
                                                             kShortcutDescActionCurveEditorHorizontal,
                                                             interpMenu);
    appPTR->getIcon(Natron::NATRON_PIXMAP_INTERP_CURVE_H, NATRON_MEDIUM_BUTTON_ICON_SIZE, &pix);
    horizontalInterpAction->setIcon(QIcon(pix));
    horizontalInterpAction->setIconVisibleInMenu(true);

    QObject::connect(horizontalInterpAction, SIGNAL(triggered()),
                     q_ptr, SLOT(horizontalInterpSelectedKeyframes()));

    interpMenu->addAction(horizontalInterpAction);

    QAction *breakInterpAction = new ActionWithShortcut(kShortcutGroupDopeSheetEditor,
                                                        kShortcutIDActionCurveEditorBreak,
                                                        kShortcutDescActionCurveEditorBreak,
                                                        interpMenu);
    appPTR->getIcon(Natron::NATRON_PIXMAP_INTERP_BREAK, NATRON_MEDIUM_BUTTON_ICON_SIZE, &pix);
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
                     q_ptr, SLOT(centerOnSelection()));
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

        double left,right;
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

void DopeSheetView::centerOn(double xMin, double xMax)
{
    _imp->zoomContext.fill(xMin, xMax, _imp->zoomContext.bottom(), _imp->zoomContext.top());

    redraw();
}

std::pair<double, double> DopeSheetView::getKeyframeRange() const
{
    std::pair<double, double> ret;

    std::vector<double> dimFirstKeys;
    std::vector<double> dimLastKeys;

    DSTreeItemNodeMap dsNodeItems = _imp->model->getItemNodeMap();

    for (DSTreeItemNodeMap::const_iterator it = dsNodeItems.begin(); it != dsNodeItems.end(); ++it) {
        if ((*it).first->isHidden()) {
            continue;
        }

        const boost::shared_ptr<DSNode>& dsNode = (*it).second;

        const DSTreeItemKnobMap& dsKnobItems = dsNode->getItemKnobMap();

        for (DSTreeItemKnobMap::const_iterator itKnob = dsKnobItems.begin(); itKnob != dsKnobItems.end(); ++itKnob) {
            if ((*itKnob).first->isHidden()) {
                continue;
            }

            const boost::shared_ptr<DSKnob>& dsKnob = (*itKnob).second;

            for (int i = 0; i < dsKnob->getKnobGui()->getKnob()->getDimension(); ++i) {
                KeyFrameSet keyframes = dsKnob->getKnobGui()->getCurve(i)->getKeyFrames_mt_safe();

                if (keyframes.empty()) {
                    continue;
                }

                dimFirstKeys.push_back(keyframes.begin()->getTime());
                dimLastKeys.push_back(keyframes.rbegin()->getTime());
            }
        }
        
        // Also append the range of the clip if this is a Reader/Group/Time node
        std::map<DSNode *, FrameRange >::const_iterator foundRange = _imp->nodeRanges.find(dsNode.get());
        if (foundRange != _imp->nodeRanges.end()) {
            const FrameRange& range = foundRange->second;
            if (!dimFirstKeys.empty()) {
                dimFirstKeys[0] = range.first;
            } else {
                dimFirstKeys.push_back(range.first);
            }
            if (!dimLastKeys.empty()) {
                dimLastKeys[0] = range.second;
            } else {
                dimLastKeys.push_back(range.second);
            }
        }
        
    }

    if (dimFirstKeys.empty() || dimLastKeys.empty()) {
        ret.first = 0;
        ret.second = 0;
    }
    else {
        ret.first = *std::min_element(dimFirstKeys.begin(), dimFirstKeys.end());
        ret.second = *std::max_element(dimLastKeys.begin(), dimLastKeys.end());
    }

    return ret;
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

void DopeSheetView::onSelectedAllTriggered()
{
    _imp->model->getSelectionModel()->selectAll();

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

void DopeSheetView::centerOnSelection()
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
    Natron::DopeSheetItemType nodeType = dsNode->getItemType();
    NodePtr node = dsNode->getInternalNode();

    bool mustComputeNodeRange = true;

    if (nodeType == Natron::eDopeSheetItemTypeCommon) {
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
    else if (nodeType == Natron::eDopeSheetItemTypeReader) {
        // The dopesheet view must refresh if the user set some values in the settings panel
        // so we connect some signals/slots
        boost::shared_ptr<KnobSignalSlotHandler> lastFrameKnob =  node->getKnobByName(kReaderParamNameLastFrame)->getSignalSlotHandler();
        assert(lastFrameKnob);
        boost::shared_ptr<KnobSignalSlotHandler> startingTimeKnob = node->getKnobByName(kReaderParamNameStartingTime)->getSignalSlotHandler();
        assert(startingTimeKnob);

        connect(lastFrameKnob.get(), SIGNAL(valueChanged(int, int)),
                this, SLOT(onRangeNodeChanged(int, int)));

        connect(startingTimeKnob.get(), SIGNAL(valueChanged(int, int)),
                this, SLOT(onRangeNodeChanged(int, int)));

        // We don't make the connection for the first frame knob, because the
        // starting time is updated when it's modified. Thus we avoid two
        // refreshes of the view.
    }
    else if (nodeType == Natron::eDopeSheetItemTypeRetime) {
        boost::shared_ptr<KnobSignalSlotHandler> speedKnob =  node->getKnobByName(kRetimeParamNameSpeed)->getSignalSlotHandler();
        assert(speedKnob);

        connect(speedKnob.get(), SIGNAL(valueChanged(int, int)),
                this, SLOT(onRangeNodeChanged(int, int)));
    }
    else if (nodeType == Natron::eDopeSheetItemTypeTimeOffset) {
        boost::shared_ptr<KnobSignalSlotHandler> timeOffsetKnob =  node->getKnobByName(kReaderParamNameTimeOffset)->getSignalSlotHandler();
        assert(timeOffsetKnob);

        connect(timeOffsetKnob.get(), SIGNAL(valueChanged(int, int)),
                this, SLOT(onRangeNodeChanged(int, int)));
    }
    else if (nodeType == Natron::eDopeSheetItemTypeFrameRange) {
        boost::shared_ptr<KnobSignalSlotHandler> frameRangeKnob =  node->getKnobByName(kFrameRangeParamNameFrameRange)->getSignalSlotHandler();
        assert(frameRangeKnob);

        connect(frameRangeKnob.get(), SIGNAL(valueChanged(int, int)),
                this, SLOT(onRangeNodeChanged(int, int)));
    }

    if (mustComputeNodeRange) {
        _imp->computeNodeRange(dsNode);
    }

    if (boost::shared_ptr<DSNode> parentGroupDSNode = _imp->model->getGroupDSNode(dsNode)) {
        _imp->computeGroupRange(parentGroupDSNode.get());
    }
}

void DopeSheetView::onNodeAboutToBeRemoved(DSNode *dsNode)
{
    if (boost::shared_ptr<DSNode> parentGroupDSNode = _imp->model->getGroupDSNode(dsNode)) {
        _imp->computeGroupRange(parentGroupDSNode.get());
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

    boost::shared_ptr<DSNode> dsNode;

    if (KnobSignalSlotHandler *knobHandler = qobject_cast<KnobSignalSlotHandler *>(signalSender)) {
        dsNode = _imp->model->findDSNode(knobHandler->getKnob());
    }
    else if (KnobGui *knobGui = qobject_cast<KnobGui *>(signalSender)) {
        dsNode = _imp->model->findDSNode(knobGui->getKnob());
    }

    if (!dsNode) {
        return;
    }

    if (boost::shared_ptr<DSNode> parentGroupDSNode = _imp->model->getGroupDSNode(dsNode.get())) {
        _imp->computeGroupRange(parentGroupDSNode.get());
    }
}

void DopeSheetView::onRangeNodeChanged(int /*dimension*/, int /*reason*/)
{
    QObject *signalSender = sender();

    boost::shared_ptr<DSNode> dsNode;

    if (KnobSignalSlotHandler *knobHandler = qobject_cast<KnobSignalSlotHandler *>(signalSender)) {
        KnobHolder *holder = knobHandler->getKnob()->getHolder();
        Natron::EffectInstance *effectInstance = dynamic_cast<Natron::EffectInstance *>(holder);
        assert(effectInstance);
        if (!effectInstance) {
            return;
        }
        dsNode = _imp->model->findDSNode(effectInstance->getNode().get());
    }

    assert(dsNode);

    _imp->computeNodeRange(dsNode.get());

    //Since this function is called a lot, let a chance to Qt to concatenate events
    //NB: updateGL() does not concatenate
    update();
}

void DopeSheetView::onHierarchyViewItemExpandedOrCollapsed(QTreeWidgetItem *item)
{
    // Compute the range rects of affected items
    if (boost::shared_ptr<DSNode> dsNode = _imp->model->findParentDSNode(item)) {
        _imp->computeRangesBelow(dsNode.get());
    }

    _imp->computeSelectedKeysBRect();

    redraw();
}

void DopeSheetView::onHierarchyViewScrollbarMoved(int /*value*/)
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

void DopeSheetView::onKeyframeSelectionChanged()
{    
    refreshSelectionBboxAndRedraw();
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
        centerOnSelection();
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

        if (!_imp->selectedKeysBRect.isNull()) {
            _imp->drawSelectedKeysBRect();
        }

        _imp->drawCurrentFrameIndicator();
    }
    glCheckError();
}

void DopeSheetView::mousePressEvent(QMouseEvent *e)
{
    running_in_main_thread();

    bool didSomething = false;

    if ( buttonDownIsRight(e) ) {
        _imp->createContextMenu();
        _imp->contextMenu->exec(mapToGlobal(e->pos()));

        e->accept();
        //didSomething = true;
        return;
    }

    if (buttonDownIsMiddle(e)) {
        _imp->eventState = DopeSheetView::esDraggingView;
        didSomething = true;
    } else if (((e->buttons() & Qt::MiddleButton) &&
                (buttonMetaAlt(e) == Qt::AltModifier || (e->buttons() & Qt::LeftButton))) ||
               ((e->buttons() & Qt::LeftButton) &&
                (buttonMetaAlt(e) == (Qt::AltModifier|Qt::MetaModifier)))) {
        // Alt + middle or Left + middle or Crtl + Alt + Left = zoom
        _imp->eventState = esZoomingView;
        _imp->lastPosOnMousePress = e->pos();
        _imp->lastPosOnMouseMove = e->pos();
        return;
    }

    QPointF clickZoomCoords = _imp->zoomContext.toZoomCoordinates(e->x(), e->y());

    if (buttonDownIsLeft(e)) {
        if (!_imp->selectedKeysBRect.isNull() && _imp->isNearbySelectedKeysBRec(e->pos())) {
            _imp->eventState = DopeSheetView::esMoveKeyframeSelection;
            didSomething = true;
        } else if (_imp->isNearByCurrentFrameIndicatorBottom(clickZoomCoords)) {
            _imp->eventState = DopeSheetView::esMoveCurrentFrameIndicator;
            didSomething = true;
        } else if (!_imp->selectedKeysBRect.isNull() && _imp->isNearbySelectedKeysBRectLeftEdge(e->pos())) {
            _imp->eventState = DopeSheetView::esTransformingKeyframesMiddleLeft;
            didSomething = true;
        } else if (!_imp->selectedKeysBRect.isNull() && _imp->isNearbySelectedKeysBRectRightEdge(e->pos())) {
            _imp->eventState = DopeSheetView::esTransformingKeyframesMiddleRight;
            didSomething = true;
        }
        
        DopeSheetSelectionModel::SelectionTypeFlags sFlags = DopeSheetSelectionModel::SelectionTypeAdd;
        if (!modCASIsShift(e)) {
            sFlags |= DopeSheetSelectionModel::SelectionTypeClear;
        }
        
        if (!didSomething) {

            ///Look for a range node
            DSTreeItemNodeMap nodes = _imp->model->getItemNodeMap();
            for (DSTreeItemNodeMap::iterator it = nodes.begin(); it!=nodes.end(); ++it) {
                if (it->second->isRangeDrawingEnabled()) {
                    
                    std::map<DSNode *, FrameRange >::const_iterator foundRange = _imp->nodeRanges.find(it->second.get());
                    if (foundRange == _imp->nodeRanges.end()) {
                        continue;
                    }
                    
                    QRectF treeItemRect = _imp->hierarchyView->visualItemRect(it->first);
                    
                    const FrameRange& range = foundRange->second;
                    RectD nodeClipRect;
                    nodeClipRect.x1 = range.first;
                    nodeClipRect.x2 = range.second;
                    nodeClipRect.y2 = _imp->zoomContext.toZoomCoordinates(0, treeItemRect.top()).y();
                    nodeClipRect.y1 = _imp->zoomContext.toZoomCoordinates(0, treeItemRect.bottom()).y();
                    
                    Natron::DopeSheetItemType nodeType = it->second->getItemType();
                    
                    if (nodeClipRect.contains(clickZoomCoords.x(), clickZoomCoords.y())) {
                        std::vector<DopeSheetKey> keysUnderMouse;
                        std::vector<boost::shared_ptr<DSNode> > selectedNodes;
                        selectedNodes.push_back(it->second);
                        
                        _imp->model->getSelectionModel()->makeSelection(keysUnderMouse, selectedNodes, sFlags);
                        
                        if (nodeType == Natron::eDopeSheetItemTypeGroup ||
                            nodeType == Natron::eDopeSheetItemTypeReader ||
                            nodeType == Natron::eDopeSheetItemTypeTimeOffset ||
                            nodeType == Natron::eDopeSheetItemTypeFrameRange) {
                            _imp->eventState = DopeSheetView::esMoveKeyframeSelection;
                            didSomething = true;
                        }
                        break;
                        
                    }
                    
                    if (nodeType == Natron::eDopeSheetItemTypeReader) {
                        _imp->currentEditedReader = it->second;
                        if (_imp->isNearByClipRectLeft(clickZoomCoords, nodeClipRect)) {
                            std::vector<DopeSheetKey> keysUnderMouse;
                            std::vector<boost::shared_ptr<DSNode> > selectedNodes;
                            selectedNodes.push_back(it->second);
                            
                            _imp->model->getSelectionModel()->makeSelection(keysUnderMouse, selectedNodes, sFlags);
                            _imp->eventState = DopeSheetView::esReaderLeftTrim;
                            didSomething = true;
                            break;
                        }
                        else if (_imp->isNearByClipRectRight(clickZoomCoords, nodeClipRect)) {
                            std::vector<DopeSheetKey> keysUnderMouse;
                            std::vector<boost::shared_ptr<DSNode> > selectedNodes;
                            selectedNodes.push_back(it->second);
                            
                            _imp->model->getSelectionModel()->makeSelection(keysUnderMouse, selectedNodes, sFlags);
                            _imp->eventState = DopeSheetView::esReaderRightTrim;
                            didSomething = true;
                            break;
                        }
                        else if (_imp->model->canSlipReader(it->second) && _imp->isNearByClipRectBottom(clickZoomCoords, nodeClipRect)) {
                            std::vector<DopeSheetKey> keysUnderMouse;
                            std::vector<boost::shared_ptr<DSNode> > selectedNodes;
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
            QTreeWidgetItem *treeItem = _imp->hierarchyView->itemAt(0, e->y());
            //Did not find a range node, look for keyframes
            if (treeItem) {
                DSTreeItemNodeMap dsNodeItems = _imp->model->getItemNodeMap();
                DSTreeItemNodeMap::const_iterator foundDsNode = dsNodeItems.find(treeItem);
                if (foundDsNode != dsNodeItems.end()) {
                    const boost::shared_ptr<DSNode> &dsNode = (*foundDsNode).second;
                    Natron::DopeSheetItemType nodeType = dsNode->getItemType();
                    if (nodeType == Natron::eDopeSheetItemTypeCommon) {
                        std::vector<DopeSheetKey> keysUnderMouse = _imp->isNearByKeyframe(dsNode, e->pos());
                        
                        if (!keysUnderMouse.empty()) {
                            std::vector<boost::shared_ptr<DSNode> > selectedNodes;
                            
                            sFlags |= DopeSheetSelectionModel::SelectionTypeRecurse;

                            _imp->model->getSelectionModel()->makeSelection(keysUnderMouse, selectedNodes, sFlags);
                            
                            _imp->eventState = DopeSheetView::esMoveKeyframeSelection;
                            didSomething = true;
                        }
                        
                    }
                } else { // if (foundDsNode != dsNodeItems.end()) {
                    //We may be on a knob row
                    boost::shared_ptr<DSKnob> dsKnob = _imp->hierarchyView->getDSKnobAt(e->pos().y());
                    if (dsKnob) {
                        std::vector<DopeSheetKey> keysUnderMouse = _imp->isNearByKeyframe(dsKnob, e->pos());
                        
                        if (!keysUnderMouse.empty()) {
                            
                            sFlags |= DopeSheetSelectionModel::SelectionTypeRecurse;

                            std::vector<boost::shared_ptr<DSNode> > selectedNodes;
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
            if (!modCASIsShift(e)) {
                _imp->model->getSelectionModel()->clearKeyframeSelection();
            }

            _imp->selectionRect.x1 = _imp->selectionRect.x2 = clickZoomCoords.x();
            _imp->selectionRect.y1 = _imp->selectionRect.y2 = clickZoomCoords.y();
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
    else if (_imp->eventState == DopeSheetView::esZoomingView) {
        _imp->zoomOrPannedSinceLastFit = true;

        int deltaX = 2 * (e->x() - _imp->lastPosOnMouseMove.x());

        const double par_min = 0.0001;
        const double par_max = 10000.;
        double scaleFactorX = std::pow( NATRON_WHEEL_ZOOM_PER_DELTA, deltaX);
        QPointF zoomCenter = _imp->zoomContext.toZoomCoordinates( _imp->lastPosOnMousePress.x(),
                                                                  _imp->lastPosOnMousePress.y());

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
        if (_imp->gui->isTripleSyncEnabled()) {
            _imp->updateCurveWidgetFrameRange();
            _imp->gui->centerOpenedViewersOn(_imp->zoomContext.left(), _imp->zoomContext.right());
        }
    }
    else if (buttonDownIsLeft(e)) {
        _imp->onMouseLeftButtonDrag(e);
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
        if (!_imp->selectionRect.isNull()) {

            std::vector<DopeSheetKey> tempSelection;
            std::vector<boost::shared_ptr<DSNode> > nodesSelection;
            _imp->createSelectionFromRect(_imp->selectionRect, &tempSelection, &nodesSelection);

            DopeSheetSelectionModel::SelectionTypeFlags sFlags = (modCASIsShift(e))
                    ? DopeSheetSelectionModel::SelectionTypeToggle
                    : DopeSheetSelectionModel::SelectionTypeAdd;

            _imp->model->getSelectionModel()->makeSelection(tempSelection, nodesSelection, sFlags);

            _imp->computeSelectedKeysBRect();
        }

        _imp->selectionRect.clear();

        mustRedraw = true;
    } else if (_imp->eventState == DopeSheetView::esMoveCurrentFrameIndicator) {
        if (_imp->gui->isDraftRenderEnabled()) {
            _imp->gui->setDraftRenderEnabled(false);
            bool autoProxyEnabled = appPTR->getCurrentSettings()->isAutoProxyEnabled();
            if (autoProxyEnabled) {
                _imp->gui->renderAllViewers();
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
}

void
DopeSheetView::mouseDoubleClickEvent(QMouseEvent *e)
{
    running_in_main_thread();
    
    if (modCASIsControl(e)) {
        
        
        
        boost::shared_ptr<DSKnob> dsKnob = _imp->hierarchyView->getDSKnobAt(e->pos().y());
        if (dsKnob) {
            std::vector<DopeSheetKey> toPaste;
            double keyframeTime = std::floor(_imp->zoomContext.toZoomCoordinates(e->pos().x(), 0).x() + 0.5);
            int dim = dsKnob->getDimension();
            
            boost::shared_ptr<KnobI> knob = dsKnob->getInternalKnob();
            
            bool hasKeyframe = false;
            for (int i = 0; i < knob->getDimension(); ++i) {
                if (i == dim || dim == -1) {
                    boost::shared_ptr<Curve> curve = knob->getGuiCurve(i);
                    KeyFrame k;
                    if (curve && curve->getKeyFrameWithTime(keyframeTime, &k)) {
                        hasKeyframe = true;
                        break;
                    }
                }
            }
            if (!hasKeyframe) {
                
                _imp->timeline->seekFrame(SequenceTime(keyframeTime), false, 0,
                                          Natron::eTimelineChangeReasonDopeSheetEditorSeek);
                
                //The value of the keyframe will be set automatically in DSPasteKeysCommand::addOrRemoveKeyframe
                KeyFrame k(keyframeTime,0);
                DopeSheetKey key(dsKnob,k);
                toPaste.push_back(key);
                _imp->model->pasteKeys(toPaste);
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

    if ( isKeybind(kShortcutGroupGlobal, kShortcutIDActionShowPaneFullScreen, modifiers, key) ) {
        TabWidget* tab = dynamic_cast<TabWidget*>(parentWidget()->parentWidget()->parentWidget());
        assert(tab);
        if (tab) {
            QKeyEvent* ev = new QKeyEvent(QEvent::KeyPress, key, modifiers);
            QCoreApplication::postEvent(tab,ev);
        }
        
    } else if (isKeybind(kShortcutGroupDopeSheetEditor, kShortcutIDActionDopeSheetEditorDeleteKeys, modifiers, key)) {
        deleteSelectedKeyframes();
    }
    else if (isKeybind(kShortcutGroupDopeSheetEditor, kShortcutIDActionDopeSheetEditorFrameSelection, modifiers, key)) {
        centerOnSelection();
    }
    else if (isKeybind(kShortcutGroupDopeSheetEditor, kShortcutIDActionDopeSheetEditorSelectAllKeyframes, modifiers, key)) {
        onSelectedAllTriggered();
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
    } else {
        QGLWidget::keyPressEvent(e);
    }
}
