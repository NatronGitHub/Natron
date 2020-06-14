/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2020 The Natron developers
 * (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
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

#ifndef NATRON_GUI_AnimationModuleViewPrivate_H
#define NATRON_GUI_AnimationModuleViewPrivate_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Gui/GuiFwd.h"

#include <QCoreApplication>

#include "Gui/AnimationModuleView.h"
#include "Gui/AnimationModuleUndoRedo.h"
#include "Gui/AnimationModuleTreeView.h"


#define KF_TEXTURES_COUNT 18

// in pixels
#define CLICK_DISTANCE_TOLERANCE 5
#define CURSOR_WIDTH 15
#define CURSOR_HEIGHT 8
#define BOUNDING_BOX_HANDLE_SIZE 4
#define XHAIR_SIZE 20

NATRON_NAMESPACE_ENTER

typedef std::set<double> TimeSet;
typedef std::map<KnobIWPtr, KnobGui *> KnobsAndGuis;


enum EventStateEnum
{
    eEventStateDraggingView = 0,
    eEventStateDraggingKeys,
    eEventStateSelectionRectCurveEditor,
    eEventStateSelectionRectDopeSheet,
    eEventStateReaderLeftTrim,
    eEventStateReaderRightTrim,
    eEventStateReaderSlip,
    eEventStateDraggingTangent,
    eEventStateDraggingTimeline,
    eEventStateZooming,
    eEventStateDraggingTopLeftBbox,
    eEventStateDraggingMidLeftBbox,
    eEventStateDraggingBtmLeftBbox,
    eEventStateDraggingMidBtmBbox,
    eEventStateDraggingBtmRightBbox,
    eEventStateDraggingMidRightBbox,
    eEventStateDraggingTopRightBbox,
    eEventStateDraggingMidTopBbox,
    eEventStateNone
};

class AnimationModuleView;
/**
 * @class An internal class used by both CurveWidget and DopeSheetView to share common implementations
 **/
class AnimationModuleViewPrivate
{

    Q_DECLARE_TR_FUNCTIONS(AnimationModuleViewPrivate)

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
    


    AnimationModuleViewPrivate(Gui* gui, AnimationModuleView* widget, const AnimationModuleBasePtr& model);

    virtual ~AnimationModuleViewPrivate();

    void drawTimelineMarkers(const ZoomContext& ctx);

    void drawSelectionRectangle();

    void drawSelectedKeyFramesBbox(bool isCurveEditor);

    enum SelectedKeyFramesRectanglePointEnum
    {
        eSelectedKeyFramesRectanglePointTopLeft,
        eSelectedKeyFramesRectanglePointTopRight,
        eSelectedKeyFramesRectanglePointBottomRight,
        eSelectedKeyFramesRectanglePointBottomLeft,
        eSelectedKeyFramesRectanglePointMidBottom,
        eSelectedKeyFramesRectanglePointMidTop,
        eSelectedKeyFramesRectanglePointMidLeft,
        eSelectedKeyFramesRectanglePointMidRight,
        eSelectedKeyFramesRectanglePointCenter
    };

    int getKeyframeTextureSize() const
    {
        return 14;
    }

    void drawTexturedKeyframe(AnimationModuleViewPrivate::KeyframeTexture textureType, const RectD &rect, bool drawDimed) const;

    void drawKeyFrameTime(const ZoomContext& ctx,  TimeValue time, const QColor& textColor, const RectD& rect) const;

    void drawSelectionRect() const;

    void renderText(const ZoomContext& ctx, double x, double y, const QString & text, const QColor & color, const QFont & font, int flags = 0) const;

    void drawCurveEditorView();

    void drawCurves();

    void drawCurveEditorScale();

    void drawDopeSheetView();

    void drawDopeSheetScale() const;

    void drawDopeSheetRows() const;

    void drawDopeSheetTreeItemRecursive(QTreeWidgetItem* item, std::list<NodeAnimPtr>* nodesRowsOrdered) const;

    void drawDopeSheetNodeRow(QTreeWidgetItem* treeItem, const NodeAnimPtr& item) const;
    void drawDopeSheetKnobRow(QTreeWidgetItem* treeItem, const KnobAnimPtr& item, DimSpec dimension, ViewSetSpec view) const;
    void drawDopeSheetAnimItemRow(QTreeWidgetItem* treeItem, const AnimItemBasePtr& item, DimSpec dimension, ViewSetSpec view) const;
    void drawDopeSheetTableItemRow(QTreeWidgetItem* treeItem, const TableItemAnimPtr& item, AnimatedItemTypeEnum type, DimSpec dimension, ViewSetSpec view) const;

    void drawDopeSheetNodeRowSeparation(const NodeAnimPtr NodeAnim) const;

    void drawDopeSheetRange(const NodeAnimPtr &NodeAnim) const;
    void drawDopeSheetKeyframes(QTreeWidgetItem* treeItem, const AnimItemBasePtr &item, DimSpec dimension, ViewSetSpec view, bool drawdimed) const;

    void generateKeyframeTextures();

    static AnimationModuleViewPrivate::KeyframeTexture kfTextureFromKeyframeType(KeyframeTypeEnum kfType, bool selected);

    bool isNearbyTimelineTopPoly(const QPoint & pt) const;

    bool isNearbyTimelineBtmPoly(const QPoint & pt) const;


    bool isNearbySelectedKeyFramesCrossWidget(const ZoomContext& ctx, const RectD& rect, const QPoint & pt) const;
    bool isNearbyBboxTopLeft(const ZoomContext& ctx, const RectD& rect, const QPoint& pt) const;
    bool isNearbyBboxMidLeft(const ZoomContext& ctx,const RectD& rect, const QPoint& pt) const;
    bool isNearbyBboxBtmLeft(const ZoomContext& ctx, const RectD& rect, const QPoint& pt) const;
    bool isNearbyBboxMidBtm(const ZoomContext& ctx, const RectD& rect, const QPoint& pt) const;
    bool isNearbyBboxBtmRight(const ZoomContext& ctx, const RectD& rect, const QPoint& pt) const;
    bool isNearbyBboxMidRight(const ZoomContext& ctx, const RectD& rect, const QPoint& pt) const;
    bool isNearbyBboxTopRight(const ZoomContext& ctx, const RectD& rect, const QPoint& pt) const;
    bool isNearbyBboxMidTop(const ZoomContext& ctx, const RectD& rect, const QPoint& pt) const;


    std::vector<CurveGuiPtr> getSelectedCurves() const;
    std::vector<CurveGuiPtr> getVisibleCurves() const;

    /**
     * @brief Returns whether the click at position pt is nearby the curve.
     * If so then the value x and y will be set to the position on the curve
     * if they are not NULL.
     **/
    CurveGuiPtr isNearbyCurve(const QPoint &pt, double* x = NULL, double *y = NULL) const;
    AnimItemDimViewKeyFrame isNearbyKeyFrame(const ZoomContext& ctx, const QPoint & pt) const;
    AnimItemDimViewKeyFrame isNearbyKeyFrameText(const QPoint& pt) const;
    std::pair<MoveTangentCommand::SelectedTangentEnum, AnimItemDimViewKeyFrame > isNearbyTangent(const QPoint & pt) const;
    std::pair<MoveTangentCommand::SelectedTangentEnum, AnimItemDimViewKeyFrame > isNearbySelectedTangentText(const QPoint & pt) const;

    void moveSelectedTangent(const QPointF & pos);

    void refreshSelectionRectangle(const ZoomContext& ctx, double x, double y);

    void keyFramesWithinRect(const RectD& canonicalRect, AnimItemDimViewKeyFramesMap* keys) const;

    void makeSelectionFromCurveEditorSelectionRectangle(bool toggleSelection);

    bool setCurveEditorCursor(const QPoint& eventPos);

    bool setDopeSheetCursor(const QPoint& eventPos);

    void createMenu(bool isCurveWidget, const QPoint& globalPos);

    // Called in the end of createMenu
    void addMenuCurveEditorMenuOptions(Menu* menu);

    /**
     * @brief A thin wrapper around AnimationModuleBase::moveSelectedKeysAndNodes that clamps keyframes motion to integers if needed
     **/
    void moveSelectedKeyFrames(const QPointF& lastCanonicalPos, const QPointF& newCanonicalPos);

    enum TransformSelectionCenterEnum
    {
        eTransformSelectionCenterMiddle,
        eTransformSelectionCenterLeft,
        eTransformSelectionCenterRight,
        eTransformSelectionCenterBottom,
        eTransformSelectionCenterTop
    };

    void transformSelectedKeyFrames(const QPointF & oldClick_opengl, const QPointF & newClick_opengl, bool shiftHeld, TransformSelectionCenterEnum centerType, bool scaleX, bool scaleY);

    void moveCurrentFrameIndicator(int frame);

    void addKey(const CurveGuiPtr& curve, double xCurve, double yCurve);

    /**
     * @brief Returns the bounding box of the keyframe texture
     **/
    RectD getKeyFrameBoundingRectCanonical(const ZoomContext& ctx, double xCanonical, double yCanonical) const;

    QRectF nameItemRectToRowRect(const QRectF &rect) const;

    ////

    Qt::CursorShape getCursorDuringHover(const QPoint &widgetCoords) const;

    bool isNearByClipRectLeft(const QPointF& zoomCoordPos, const RectD &clipRect) const;
    bool isNearByClipRectRight(const QPointF& zoomCoordPos, const RectD &clipRect) const;
    bool isNearByClipRectBottom(const QPointF& zoomCoordPos, const RectD &clipRect) const;
    bool isNearByCurrentFrameIndicatorBottom(const QPointF &zoomCoords) const;


    KeyFrameSet isNearByKeyframe(const AnimItemBasePtr &item, DimSpec dimension, ViewSetSpec view, const QPointF &widgetCoords) const;

    void drawDopeSheetGroupOverlay(const NodeAnimPtr &NodeAnim, const NodeAnimPtr &group) const;

    void refreshDopeSheetSelectedKeysBRect();

    void refreshCurveEditorSelectedKeysBRect();

    void computeRangesBelow(const NodeAnimPtr& NodeAnim);
    

    void makeSelectionFromDopeSheetSelectionRectangle(bool toggleSelection);

    void makeSelectionFromDopeSheetSelectionRectangleInternal(const RectD &rect, AnimItemDimViewKeyFramesMap *result, std::vector<NodeAnimPtr>* selectedNodes, std::vector<TableItemAnimPtr>* selectedItems);

    void createSelectionFromRectRecursive(const RectD &rect, QTreeWidgetItem* item, AnimItemDimViewKeyFramesMap *result, std::vector<NodeAnimPtr>* selectedNodes, std::vector<TableItemAnimPtr>* selectedItems);

    void checkAnimItemInRectInternal(const RectD& rect, QTreeWidgetItem* item, const AnimItemBasePtr& knob, ViewIdx view, DimIdx dimension, AnimItemDimViewKeyFramesMap *result, std::vector<NodeAnimPtr>* selectedNodes, std::vector<TableItemAnimPtr>* selectedItems);
    void checkAnimItemInRect(const RectD& rect, QTreeWidgetItem* item, const AnimItemBasePtr& knob, ViewSetSpec view, DimSpec dimension, AnimItemDimViewKeyFramesMap *result, std::vector<NodeAnimPtr>* selectedNodes, std::vector<TableItemAnimPtr>* selectedItems);
    void checkTableItemAnimInRect(const RectD& rect, QTreeWidgetItem* item, const TableItemAnimPtr& knob, AnimatedItemTypeEnum type, ViewSetSpec view, DimSpec dimension, AnimItemDimViewKeyFramesMap *result, std::vector<NodeAnimPtr>* selectedNodes, std::vector<TableItemAnimPtr>* selectedItems);
    void checkNodeAnimInRect(const RectD& rect, QTreeWidgetItem* item, const NodeAnimPtr& knob, AnimItemDimViewKeyFramesMap *result, std::vector<NodeAnimPtr>* selectedNodes, std::vector<TableItemAnimPtr>* selectedItems);

    void zoomView(const QPointF& evPos, int deltaX, int deltaY, ZoomContext& zoomCtx, bool canMoveX, bool canZoomY);

    bool curveEditorDoubleClickEvent(QMouseEvent* e);
    bool curveEditorMousePressEvent(QMouseEvent* e);

    bool dopeSheetDoubleClickEvent(QMouseEvent* e);
    bool dopeSheetMousePressEvent(QMouseEvent* e);

    bool dopeSheetAddKeyFrame(const QPoint& p);

public:

    // ptr to the widget
    AnimationModuleView* _publicInterface;

    Gui* _gui;

    // weak ref to the animation module
    AnimationModuleBaseWPtr _model;

    // protects zoomCtx for serialization thread
    mutable QMutex zoomCtxMutex;
    ZoomContext curveEditorZoomContext, dopeSheetZoomContext;
    bool zoomOrPannedSinceLastFit;

    TextRenderer textRenderer;

    QSize sizeH;

    // for keyframe selection
    // This is in zoom coordinates...
    RectD selectionRect;

    // keyframe selection rect
    // This is in zoom coordinates
    RectD curveEditorSelectedKeysBRect, dopeSheetSelectedKeysBRect;

    // keframe type textures
    GLuint kfTexturesIDs[KF_TEXTURES_COUNT];

    GLuint savedTexture;

    bool drawnOnce;

    // the last mouse press pos in widget coordinates
    QPointF dragStartPoint;

    // the last mouse press or move, in widget coordinates
    QPoint lastMousePos;

    // True if drag orientation must be computed in mouse event
    bool mustSetDragOrientation;

    // used to drag a key frame in only 1 direction (horizontal or vertical)
    QPoint mouseDragOrientation;

    // The last delta in mouseMoveEvent
    QPointF keyDragLastMovement;

    // Interaction State
    EventStateEnum state;

    // True if the state was set by interacting with curve editor
    bool eventTriggeredFromCurveEditor;

    // If a derivative is selected, this points to it
    std::pair<MoveTangentCommand::SelectedTangentEnum, AnimItemDimViewKeyFrame> selectedDerivative;

    // Pointer to the tree view
    AnimationModuleTreeView *treeView;

    // Selected reader
    NodeAnimPtr currentEditedReader;

    OverlayInteractBaseWPtr customInteract;

};

NATRON_NAMESPACE_EXIT
#endif // NATRON_GUI_AnimationModuleViewPrivate_H
