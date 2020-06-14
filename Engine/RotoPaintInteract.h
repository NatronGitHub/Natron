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

#ifndef ROTOPAINTINTERACT_H
#define ROTOPAINTINTERACT_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <list>
#include <QtCore/QPointF>
#include <QtCore/QRectF>

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/enable_shared_from_this.hpp>
#endif

#include <ofxNatron.h>

#include "Engine/EngineFwd.h"
#include "Engine/BezierCP.h"
#include "Engine/Bezier.h"

NATRON_NAMESPACE_ENTER


#define kControlPointMidSize 3
#define kBezierSelectionTolerance 8
#define kControlPointSelectionTolerance 8
#define kXHairSelectedCpsTolerance 8
#define kXHairSelectedCpsBox 8
#define kTangentHandleSelectionTolerance 8
#define kTransformArrowLenght 10
#define kTransformArrowWidth 3
#define kTransformArrowOffsetFromPoint 15


// parameters


// The toolbar
#define kRotoUIParamToolbar "Toolbar"

#define kRotoUIParamSelectionToolButton "SelectionToolButton"
#define kRotoUIParamSelectionToolButtonLabel "Selection Tool"

#define kRotoUIParamSelectAllToolButtonAction "SelectAllTool"
#define kRotoUIParamSelectAllToolButtonActionLabel "Select All Tool"
#define kRotoUIParamSelectAllToolButtonActionHint "Everything can be selected and moved"

#define kRotoUIParamSelectPointsToolButtonAction "SelectPointsTool"
#define kRotoUIParamSelectPointsToolButtonActionLabel "Select Points Tool"
#define kRotoUIParamSelectPointsToolButtonActionHint "Works only for the points of the inner shape," \
    " feather points will not be taken into account."

#define kRotoUIParamSelectShapesToolButtonAction "SelectShapesTool"
#define kRotoUIParamSelectShapesToolButtonActionLabel "Select Shapes Tool"
#define kRotoUIParamSelectShapesToolButtonActionHint "Only Shapes may be selected"

#define kRotoUIParamSelectFeatherPointsToolButtonAction "SelectFeatherTool"
#define kRotoUIParamSelectFeatherPointsToolButtonActionLabel "Select Feather Points Tool"
#define kRotoUIParamSelectFeatherPointsToolButtonActionHint "Only Feather points may be selected"

#define kRotoUIParamEditPointsToolButton "EditPointsToolButton"
#define kRotoUIParamEditPointsToolButtonLabel "Points Edition Tool"

#define kRotoUIParamAddPointsToolButtonAction "AddPointsTool"
#define kRotoUIParamAddPointsToolButtonActionLabel "Add Points Tool"
#define kRotoUIParamAddPointsToolButtonActionHint "Add a control point to the shape"

#define kRotoUIParamRemovePointsToolButtonAction "RemovePointsTool"
#define kRotoUIParamRemovePointsToolButtonActionLabel "Remove Points Tool"
#define kRotoUIParamRemovePointsToolButtonActionHint "Remove a control point from the shape"

#define kRotoUIParamCuspPointsToolButtonAction "CuspPointsTool"
#define kRotoUIParamCuspPointsToolButtonActionLabel "Cusp Points Tool"
#define kRotoUIParamCuspPointsToolButtonActionHint "Cusp points on the shape"

#define kRotoUIParamSmoothPointsToolButtonAction "SmoothPointsTool"
#define kRotoUIParamSmoothPointsToolButtonActionLabel "Smooth Points Tool"
#define kRotoUIParamSmoothPointsToolButtonActionHint "Smooth points on the shape"

#define kRotoUIParamOpenCloseCurveToolButtonAction "OpenCloseShapeTool"
#define kRotoUIParamOpenCloseCurveToolButtonActionLabel "Open/Close Shape Tool"
#define kRotoUIParamOpenCloseCurveToolButtonActionHint "Open or Close shapes"

#define kRotoUIParamRemoveFeatherToolButtonAction "RemoveFeatherPointTool"
#define kRotoUIParamRemoveFeatherToolButtonActionLabel "Remove Feather Tool"
#define kRotoUIParamRemoveFeatherToolButtonActionHint "Remove feather on points"

#define kRotoUIParamBezierEditionToolButton "EditBezierToolButton"
#define kRotoUIParamBezierEditionToolButtonLabel "Shapes Drawing Tool"

#define kRotoUIParamDrawBezierToolButtonAction "DrawBezierTool"
#define kRotoUIParamDrawBezierToolButtonActionLabel "Bezier Tool"
#define kRotoUIParamDrawBezierToolButtonActionHint "Draw Bezier"

#define kRotoUIParamDrawEllipseToolButtonAction "DrawEllipseTool"
#define kRotoUIParamDrawEllipseToolButtonActionLabel "Ellipse Tool"
#define kRotoUIParamDrawEllipseToolButtonActionHint "Draw Ellipse"

#define kRotoUIParamDrawRectangleToolButtonAction "DrawRectangleTool"
#define kRotoUIParamDrawRectangleToolButtonActionLabel "Rectangle Tool"
#define kRotoUIParamDrawRectangleToolButtonActionHint "Draw Rectangle"


#define kRotoUIParamPaintBrushToolButton "PaintBrushToolButton"
#define kRotoUIParamPaintBrushToolButtonLabel "Paint Brush Tool"

#define kRotoUIParamDrawBrushToolButtonAction "PaintSolidTool"
#define kRotoUIParamDrawBrushToolButtonActionLabel "Solid Paint Brush Tool"
#define kRotoUIParamDrawBrushToolButtonActionHint "Draw using the pen"

#define kRotoUIParamPencilToolButtonAction "PencilTool"
#define kRotoUIParamPencilToolButtonActionLabel "Pencil Tool"
#define kRotoUIParamPencilToolButtonActionHint "Draw open bezier"

#define kRotoUIParamEraserToolButtonAction "EraserTool"
#define kRotoUIParamEraserToolButtonActionLabel "Eraser Tool"
#define kRotoUIParamEraserToolButtonActionHint "Use the Eraser"

#define kRotoUIParamCloneBrushToolButton "CloneToolButton"
#define kRotoUIParamCloneBrushToolButtonLabel "Clone Tool"

#define kRotoUIParamCloneToolButtonAction "CloneTool"
#define kRotoUIParamCloneToolButtonActionLabel "Clone Tool"
#define kRotoUIParamCloneToolButtonActionHint "Draw with the pen to clone areas of the image. Hold CTRL to move the clone offset."

#define kRotoUIParamRevealToolButtonAction "RevealTool"
#define kRotoUIParamRevealToolButtonActionLabel "Reveal Tool"
#define kRotoUIParamRevealToolButtonActionHint "Draw with the pen to apply the effect from the selected source. Hold CTRL to move the offset"

#define kRotoUIParamEffectBrushToolButton "EffectToolButton"
#define kRotoUIParamEffectBrushToolButtonLabel "Effect Tool"

#define kRotoUIParamBlurToolButtonAction "BlurTool"
#define kRotoUIParamBlurToolButtonActionLabel "Blur Tool"
#define kRotoUIParamBlurToolButtonActionHint "Draw with the pen to apply a blur"

#define kRotoUIParamSmearToolButtonAction "SmearTool"
#define kRotoUIParamSmearToolButtonActionLabel "Smear Tool"
#define kRotoUIParamSmearToolButtonActionHint "Draw with the pen to blur and displace a portion of the source image along the direction of the pen"

#define kRotoUIParamMergeBrushToolButton "MergeToolButton"
#define kRotoUIParamMergeBrushToolButtonLabel "Merging Tool"

#define kRotoUIParamDodgeToolButtonAction "DodgeTool"
#define kRotoUIParamDodgeToolButtonActionLabel "Dodge Tool"
#define kRotoUIParamDodgeToolButtonActionHint "Make the source image brighter"

#define kRotoUIParamBurnToolButtonAction "BurnTool"
#define kRotoUIParamBurnToolButtonActionLabel "Burn Tool"
#define kRotoUIParamBurnToolButtonActionHint "Make the source image darker"

// The right click menu
#define kRotoUIParamRightClickMenu kNatronOfxParamRightClickMenu

//For all items
#define kRotoUIParamRightClickMenuActionRemoveItems "removeItemsAction"
#define kRotoUIParamRightClickMenuActionRemoveItemsLabel "Remove Selected Item(s)"

#define kRotoUIParamRightClickMenuActionCuspItems "cuspItemsAction"
#define kRotoUIParamRightClickMenuActionCuspItemsLabel "Cusp Selected Item(s)"

#define kRotoUIParamRightClickMenuActionSmoothItems "SmoothItemsAction"
#define kRotoUIParamRightClickMenuActionSmoothItemsLabel "Smooth Selected Item(s)"

#define kRotoUIParamRightClickMenuActionRemoveItemsFeather "removeItemsFeatherAction"
#define kRotoUIParamRightClickMenuActionRemoveItemsFeatherLabel "Remove Selected item(s) Feather"


#define kRotoUIParamRightClickMenuActionNudgeLeft "nudgeLeftAction"
#define kRotoUIParamRightClickMenuActionNudgeLeftLabel "Nudge Left"

#define kRotoUIParamRightClickMenuActionNudgeRight "nudgeRightAction"
#define kRotoUIParamRightClickMenuActionNudgeRightLabel "Nudge Right"

#define kRotoUIParamRightClickMenuActionNudgeBottom "nudgeBottomAction"
#define kRotoUIParamRightClickMenuActionNudgeBottomLabel "Nudge Bottom"

#define kRotoUIParamRightClickMenuActionNudgeTop "nudgeTopAction"
#define kRotoUIParamRightClickMenuActionNudgeTopLabel "Nudge Top"

// just for shapes
#define kRotoUIParamRightClickMenuActionSelectAll "selectAllAction"
#define kRotoUIParamRightClickMenuActionSelectAllLabel "Select All"

#define kRotoUIParamRightClickMenuActionOpenClose "openCloseAction"
#define kRotoUIParamRightClickMenuActionOpenCloseLabel "Open/Close Shape"

#define kRotoUIParamRightClickMenuActionLockShapes "lockShapesAction"
#define kRotoUIParamRightClickMenuActionLockShapesLabel "Lock Shape(s)"

// Viewer UI buttons

// Roto
#define kRotoUIParamAutoKeyingEnabled "autoKeyingEnabledButton"
#define kRotoUIParamAutoKeyingEnabledLabel "Enable Auto-Keying"
#define kRotoUIParamAutoKeyingEnabledHint "When activated any change made to a control point will set a keyframe at the current time"

#define kRotoUIParamFeatherLinkEnabled "featherLinkEnabledButton"
#define kRotoUIParamFeatherLinkEnabledLabel "Enable Feather-Link"
#define kRotoUIParamFeatherLinkEnabledHint "Feather-link: When activated the feather points will follow the same" \
    " movement as their counter-part does"

#define kRotoUIParamDisplayFeather "displayFeatherButton"
#define kRotoUIParamDisplayFeatherLabel "Display Feather"
#define kRotoUIParamDisplayFeatherHint "When checked, the feather curve applied to the shape(s) will be visible and editable"

#define kRotoUIParamStickySelectionEnabled "stickySelectionEnabledButton"
#define kRotoUIParamStickySelectionEnabledLabel "Enable Sticky Selection"
#define kRotoUIParamStickySelectionEnabledHint "Sticky-selection: When activated, " \
    " clicking outside of any shape will not clear the current selection"

#define kRotoUIParamStickyBbox "stickyBboxButton"
#define kRotoUIParamStickyBboxLabel "Enable Sticky Bounding Box"
#define kRotoUIParamStickyBboxHint "Easy bounding box manipulation: When activated, " \
    " clicking inside of the bounding box of selected points will move the points." \
    "When deactivated, only clicking on the cross will move the points"

#define kRotoUIParamRippleEdit "rippleEditButton"
#define kRotoUIParamRippleEditLabel "Enable Ripple Edit"
#define kRotoUIParamRippleEditLabelHint "Ripple-edit: When activated, moving a control point" \
    " will move it by the same amount for all the keyframes " \
    "it has"

#define kRotoUIParamAddKeyFrame "addKeyframeButton"
#define kRotoUIParamAddKeyFrameLabel "Add Keyframe"
#define kRotoUIParamAddKeyFrameHint "Set a keyframe at the current time for the selected shape(s), if any"

#define kRotoUIParamRemoveKeyframe "removeKeyframeButton"
#define kRotoUIParamRemoveKeyframeLabel "Remove Keyframe"
#define kRotoUIParamRemoveKeyframeHint "Remove a keyframe at the current time for the selected shape(s), if any"

#define kRotoUIParamShowTransform "showTransformButton"
#define kRotoUIParamShowTransformLabel "Show Transform Handle"
#define kRotoUIParamShowTransformHint "When unchecked, even if the Transform tab is visible in the settings panel, the transform handle will be hidden"

// RotoPaint
#define kRotoUIParamColorWheel "strokeColorButton"
#define kRotoUIParamColorWheelLabel "Stroke Color"
#define kRotoUIParamColorWheelHint "The color of the next paint brush stroke to be painted"

#define kRotoUIParamBlendingOp "blendingModeButton"
#define kRotoUIParamBlendingOpLabel "Blending Mode"
#define kRotoUIParamBlendingOpHint "The blending mode of the next brush stroke"

#define kRotoUIParamOpacity "opacitySpinbox"
#define kRotoUIParamOpacityLabel "Opacity"
#define kRotoUIParamOpacityHint "The opacity of the next brush stroke to be painted. Use CTRL + SHIFT + drag " \
    "with the mouse to change the opacity."

#define kRotoUIParamPressureOpacity "pressureOpacityButton"
#define kRotoUIParamPressureOpacityLabel "Pressure Affects Opacity"
#define kRotoUIParamPressureOpacityHint "If checked, the pressure of the pen will dynamically alter the opacity of the next " \
    "brush stroke."

#define kRotoUIParamSize "sizeSpinbox"
#define kRotoUIParamSizeLabel "Brush Size"
#define kRotoUIParamSizeHint "The size of the next brush stroke to be painted. Use SHIFT + drag with the mouse " \
    "to change the size."

#define kRotoUIParamPressureSize "pressureSizeButton"
#define kRotoUIParamPressureSizeLabel "Pressure Affects Size"
#define kRotoUIParamPressureSizeHint "If checked, the pressure of the pen will dynamically alter the size of the next " \
    "brush stroke."

#define kRotoUIParamHardness "hardnessSpinbox"
#define kRotoUIParamHardnessLabel "Brush Hardness"
#define kRotoUIParamHardnessHint "The hardness of the next brush stroke to be painted"

#define kRotoUIParamPressureHardness "pressureHardnessButton"
#define kRotoUIParamPressureHardnessLabel "Pressure Affects Hardness"
#define kRotoUIParamPressureHardnessHint "If checked, the pressure of the pen will dynamically alter the hardness of the next " \
    "brush stroke"

#define kRotoUIParamBuildUp "buildUpButton"
#define kRotoUIParamBuildUpLabel "Build-Up"
#define kRotoUIParamBuildUpHint "When build-up is enabled, the next brush stroke will build up " \
    "when painted over itself"

#define kRotoUIParamEffect "effectSpinbox"
#define kRotoUIParamEffectLabel "Effect Strength"
#define kRotoUIParamEffectHint "The strength of the next paint brush effect"

#define kRotoUIParamTimeOffset "timeOffsetSpinbox"
#define kRotoUIParamTimeOffsetLabel "Time Offset"
#define kRotoUIParamTimeOffsetHint "When the Clone tool is used, this determines depending on the time offset " \
    "mode the source frame to clone. When in absolute mode, this is the frame " \
    "number of the source, when in relative mode, this is an offset relative " \
    "to the current frame"

#define kRotoUIParamTimeOffsetMode "timeOffsetModeChoice"
#define kRotoUIParamTimeOffsetModeLabel "Time Offset Mode"
#define kRotoUIParamTimeOffsetModeHint "When in absolute mode, this is the frame number of the source, " \
    "when in relative mode, this is an offset relative to " \
    "the current frame" \

#define kRotoUIParamSourceType "sourceTypeChoice"
#define kRotoUIParamSourceTypeLabel "Source"
#define kRotoUIParamSourceTypeHint "Source color used for painting the stroke when the Reveal/Clone tools are used:\n" \
    "- foreground: the painted result at this point in the hierarchy,\n" \
    "- background: the original image unpainted connected to bg,\n" \
    "- backgroundN: the original image unpainted connected to bgN"

#define kRotoUIParamResetCloneOffset "resetCloneOffsetButton"
#define kRotoUIParamResetCloneOffsetLabel "Reset Transform"
#define kRotoUIParamResetCloneOffsetHint "Reset the transform applied before cloning to identity"

#define kRotoUIParamMultiStrokeEnabled "multiStrokeEnabledButton"
#define kRotoUIParamMultiStrokeEnabledLabel "Multi-Stroke"
#define kRotoUIParamMultiStrokeEnabledHint "When checked, strokes will be appended to the same item " \
    "in the hierarchy as long as the same tool is selected.\n" \
    "Select another tool to make a new item."


// Shortcuts

#define kShortcutIDActionRotoDelete "delete"
#define kShortcutDescActionRotoDelete "Delete Element"

#define kShortcutIDActionRotoCloseBezier "closeBezier"
#define kShortcutDescActionRotoCloseBezier "Close Bezier"

#define kShortcutIDActionRotoSelectAll "selectAll"
#define kShortcutDescActionRotoSelectAll "Select All"

#define kShortcutIDActionRotoSelectionTool "selectionTool"
#define kShortcutDescActionRotoSelectionTool "Switch to Selection Mode"

#define kShortcutIDActionRotoAddTool "addTool"
#define kShortcutDescActionRotoAddTool "Switch to Add Mode"

#define kShortcutIDActionRotoEditTool "editTool"
#define kShortcutDescActionRotoEditTool "Switch to Edition Mode"

#define kShortcutIDActionRotoBrushTool "brushTool"
#define kShortcutDescActionRotoBrushTool "Switch to Brush Mode"

#define kShortcutIDActionRotoCloneTool "cloneTool"
#define kShortcutDescActionRotoCloneTool "Switch to Clone Mode"

#define kShortcutIDActionRotoEffectTool "EffectTool"
#define kShortcutDescActionRotoEffectTool "Switch to Effect Mode"

#define kShortcutIDActionRotoColorTool "colorTool"
#define kShortcutDescActionRotoColorTool "Switch to Color Mode"

#define kShortcutIDActionRotoNudgeLeft "nudgeLeft"
#define kShortcutDescActionRotoNudgeLeft "Move Bezier to the Left"

#define kShortcutIDActionRotoNudgeRight "nudgeRight"
#define kShortcutDescActionRotoNudgeRight "Move Bezier to the Right"

#define kShortcutIDActionRotoNudgeBottom "nudgeBottom"
#define kShortcutDescActionRotoNudgeBottom "Move Bezier to the Bottom"

#define kShortcutIDActionRotoNudgeTop "nudgeTop"
#define kShortcutDescActionRotoNudgeTop "Move Bezier to the Top"

#define kShortcutIDActionRotoSmooth "smooth"
#define kShortcutDescActionRotoSmooth "Smooth Bezier"

#define kShortcutIDActionRotoCuspBezier "cusp"
#define kShortcutDescActionRotoCuspBezier "Cusp Bezier"

#define kShortcutIDActionRotoRemoveFeather "rmvFeather"
#define kShortcutDescActionRotoRemoveFeather "Remove Feather"

#define kShortcutIDActionRotoLockCurve "lock"
#define kShortcutDescActionRotoLockCurve "Lock Shape"

class RotoPaintInteract;
struct RotoPaintPrivate
{
    RotoPaint* publicInterface;
    bool isPaintByDefault;
    KnobBoolWPtr premultKnob;
    KnobBoolWPtr enabledKnobs[4];
    RotoPaintInteractPtr ui;

    RotoPaintPrivate(RotoPaint* publicInterface,
                     bool isPaintByDefault);
};

///A list of points and their counter-part, that is: either a control point and its feather point, or
///the feather point and its associated control point
typedef std::pair<BezierCPPtr, BezierCPPtr> SelectedCP;
typedef std::list<SelectedCP > SelectedCPs;
typedef std::list<RotoDrawableItemPtr> SelectedItems;

enum EventStateEnum
{
    eEventStateNone = 0,
    eEventStateDraggingControlPoint,
    eEventStateDraggingSelectedControlPoints,
    eEventStateBuildingBezierControlPointTangent,
    eEventStateBuildingEllipse,
    eEventStateBuildingRectangle,
    eEventStateDraggingLeftTangent,
    eEventStateDraggingRightTangent,
    eEventStateDraggingFeatherBar,
    eEventStateDraggingBBoxTopLeft,
    eEventStateDraggingBBoxTopRight,
    eEventStateDraggingBBoxBtmRight,
    eEventStateDraggingBBoxBtmLeft,
    eEventStateDraggingBBoxMidTop,
    eEventStateDraggingBBoxMidRight,
    eEventStateDraggingBBoxMidBtm,
    eEventStateDraggingBBoxMidLeft,
    eEventStateBuildingStroke,
    eEventStateDraggingCloneOffset,
    eEventStateDraggingBrushSize,
    eEventStateDraggingBrushOpacity,
};

enum HoverStateEnum
{
    eHoverStateNothing = 0,
    eHoverStateBboxTopLeft,
    eHoverStateBboxTopRight,
    eHoverStateBboxBtmRight,
    eHoverStateBboxBtmLeft,
    eHoverStateBboxMidTop,
    eHoverStateBboxMidRight,
    eHoverStateBboxMidBtm,
    eHoverStateBboxMidLeft,
    eHoverStateBbox
};

enum SelectedCpsTransformModeEnum
{
    eSelectedCpsTransformModeTranslateAndScale = 0,
    eSelectedCpsTransformModeRotateAndSkew = 1
};

enum RotoRoleEnum
{
    eRotoRoleSelection = 0,
    eRotoRolePointsEdition,
    eRotoRoleBezierEdition,
    eRotoRolePaintBrush,
    eRotoRoleCloneBrush,
    eRotoRoleEffectBrush,
    eRotoRoleMergeBrush
};

enum RotoToolEnum
{
    eRotoToolSelectAll = 0,
    eRotoToolSelectPoints,
    eRotoToolSelectCurves,
    eRotoToolSelectFeatherPoints,

    eRotoToolAddPoints,
    eRotoToolRemovePoints,
    eRotoToolRemoveFeatherPoints,
    eRotoToolOpenCloseCurve,
    eRotoToolSmoothPoints,
    eRotoToolCuspPoints,

    eRotoToolDrawBezier,
    eRotoToolDrawBSpline,
    eRotoToolDrawEllipse,
    eRotoToolDrawRectangle,

    eRotoToolSolidBrush,
    eRotoToolOpenBezier,
    eRotoToolEraserBrush,

    eRotoToolClone,
    eRotoToolReveal,

    eRotoToolBlur,
    eRotoToolSharpen,
    eRotoToolSmear,

    eRotoToolDodge,
    eRotoToolBurn
};

class RotoPaintInteract
    : public boost::enable_shared_from_this<RotoPaintInteract>
{
public:
    RotoPaintPrivate* p;
    SelectedItems selectedItems;
    SelectedCPs selectedCps;
    QRectF selectedCpsBbox;
    bool showCpsBbox;

    ////This is by default eSelectedCpsTransformModeTranslateAndScale. When clicking the cross-hair in the center this will toggle the transform mode
    ////like it does in inkscape.
    SelectedCpsTransformModeEnum transformMode;
    BezierPtr builtBezier; //< the bezier currently being built
    BezierPtr bezierBeingDragged;
    SelectedCP cpBeingDragged; //< the cp being dragged
    BezierCPPtr tangentBeingDragged; //< the control point whose tangent is being dragged.
    //only relevant when the state is DRAGGING_X_TANGENT
    SelectedCP featherBarBeingDragged, featherBarBeingHovered;
    RotoStrokeItemPtr strokeBeingPaint;
    int strokeBeingPaintedTimelineFrame;// the frame at which we painted the last brush stroke
    std::pair<double, double> cloneOffset;
    QPointF click; // used for drawing ellipses and rectangles, to handle center/constrain. May also be used for the selection bbox.
    RotoToolEnum selectedTool;
    RotoRoleEnum selectedRole;
    KnobButtonWPtr lastPaintToolAction;
    EventStateEnum state;
    HoverStateEnum hoverState;
    QPointF lastClickPos;
    QPointF lastMousePos;
    bool evaluateOnPenUp; //< if true the next pen up will call context->evaluateChange()
    bool evaluateOnKeyUp;  //< if true the next key up will call context->evaluateChange()
    bool iSelectingwithCtrlA;
    int shiftDown;
    int ctrlDown;
    int altDown;
    bool lastTabletDownTriggeredEraser;
    QPointF mouseCenterOnSizeChange;


    //////// Toolbar
    KnobPageWPtr toolbarPage;
    KnobGroupWPtr selectedToolRole;
    KnobButtonWPtr selectedToolAction;
    KnobGroupWPtr selectToolGroup;
    KnobButtonWPtr selectAllAction;
    KnobButtonWPtr selectPointsAction;
    KnobButtonWPtr selectCurvesAction;
    KnobButtonWPtr selectFeatherPointsAction;
    KnobGroupWPtr pointsEditionToolGroup;
    KnobButtonWPtr addPointsAction;
    KnobButtonWPtr removePointsAction;
    KnobButtonWPtr cuspPointsAction;
    KnobButtonWPtr smoothPointsAction;
    KnobButtonWPtr openCloseCurveAction;
    KnobButtonWPtr removeFeatherAction;
    KnobGroupWPtr bezierEditionToolGroup;
    KnobButtonWPtr drawBezierAction;
    KnobButtonWPtr drawEllipseAction;
    KnobButtonWPtr drawRectangleAction;
    KnobGroupWPtr paintBrushToolGroup;
    KnobButtonWPtr brushAction;
    KnobButtonWPtr pencilAction;
    KnobButtonWPtr eraserAction;
    KnobGroupWPtr cloneBrushToolGroup;
    KnobButtonWPtr cloneAction;
    KnobButtonWPtr revealAction;
    KnobGroupWPtr effectBrushToolGroup;
    KnobButtonWPtr blurAction;
    KnobButtonWPtr smearAction;
    KnobGroupWPtr mergeBrushToolGroup;
    KnobButtonWPtr dodgeAction;
    KnobButtonWPtr burnAction;

    //////Right click menu
    KnobChoiceWPtr rightClickMenuKnob;

    //Right click on point
    KnobButtonWPtr removeItemsMenuAction;
    KnobButtonWPtr cuspItemMenuAction;
    KnobButtonWPtr smoothItemMenuAction;
    KnobButtonWPtr removeItemFeatherMenuAction;
    KnobButtonWPtr nudgeLeftMenuAction, nudgeRightMenuAction, nudgeBottomMenuAction, nudgeTopMenuAction;

    // Right click on curve
    KnobButtonWPtr selectAllMenuAction;
    KnobButtonWPtr openCloseMenuAction;
    KnobButtonWPtr lockShapeMenuAction;

    // Roto buttons
    KnobButtonWPtr autoKeyingEnabledButton;
    KnobButtonWPtr featherLinkEnabledButton;
    KnobButtonWPtr displayFeatherEnabledButton;
    KnobButtonWPtr stickySelectionEnabledButton;
    KnobButtonWPtr bboxClickAnywhereButton;
    KnobButtonWPtr rippleEditEnabledButton;
    KnobButtonWPtr addKeyframeButton;
    KnobButtonWPtr removeKeyframeButton;
    KnobButtonWPtr showTransformHandle;

    // RotoPaint buttons
    KnobColorWPtr colorWheelButton;
    KnobChoiceWPtr compositingOperatorChoice;
    KnobDoubleWPtr opacitySpinbox;
    KnobButtonWPtr pressureOpacityButton;
    KnobDoubleWPtr sizeSpinbox;
    KnobButtonWPtr pressureSizeButton;
    KnobDoubleWPtr hardnessSpinbox;
    KnobButtonWPtr pressureHardnessButton;
    KnobButtonWPtr buildUpButton;
    KnobDoubleWPtr effectSpinBox;
    KnobIntWPtr timeOffsetSpinBox;
    KnobChoiceWPtr timeOffsetModeChoice;
    KnobChoiceWPtr sourceTypeChoice;
    KnobButtonWPtr resetCloneOffsetButton;
    KnobBoolWPtr multiStrokeEnabled;


private:
    struct MakeSharedEnabler;

    // constructors should be privatized in any class that derives from boost::enable_shared_from_this<>

    RotoPaintInteract(RotoPaintPrivate* p);

public:
    static RotoPaintInteractPtr create(RotoPaintPrivate* p);

    bool isFeatherVisible() const;

    RotoContextPtr getContext();

    RotoToolEnum getSelectedTool() const
    {
        return selectedTool;
    }

    bool isStickySelectionEnabled() const;

    bool isMultiStrokeEnabled() const;

    bool getRoleForGroup(const KnobGroupPtr& group, RotoRoleEnum* role) const;
    bool getToolForAction(const KnobButtonPtr& action, RotoToolEnum* tool) const;

    bool onRoleChangedInternal(const KnobGroupPtr& roleGroup);

    bool onToolChangedInternal(const KnobButtonPtr& actionButton);

    void clearSelection();

    void clearCPSSelection();

    void clearBeziersSelection();

    bool hasSelection() const;

    void onCurveLockedChangedRecursive(const RotoItemPtr & item, bool* ret);

    bool removeItemFromSelection(const RotoDrawableItemPtr& b);

    void computeSelectedCpsBBOX();

    QPointF getSelectedCpsBBOXCenter();

    void drawSelectedCpsBBOX();

    void drawEllipse(double x,
                     double y,
                     double radiusX,
                     double radiusY,
                     int l,
                     double r,
                     double g,
                     double b,
                     double a);

    ///by default draws a vertical arrow, which can be rotated by rotate amount.
    void drawArrow(double centerX, double centerY, double rotate, bool hovered, const std::pair<double, double> & pixelScale);

    ///same as drawArrow but the two ends will make an angle of 90 degrees
    void drawBendedArrow(double centerX, double centerY, double rotate, bool hovered, const std::pair<double, double> & pixelScale);

    void handleBezierSelection(const BezierPtr & curve);

    void handleControlPointSelection(const std::pair<BezierCPPtr, BezierCPPtr> & p);

    void drawSelectedCp(double time,
                        const BezierCPPtr & cp,
                        double x, double y,
                        const Transform::Matrix3x3& transform);

    std::pair<BezierCPPtr, BezierCPPtr>isNearbyFeatherBar(double time, const std::pair<double, double> & pixelScale, const QPointF & pos) const;

    bool isNearbySelectedCpsCrossHair(const QPointF & pos) const;

    bool isWithinSelectedCpsBBox(const QPointF& pos) const;

    bool isNearbyBBoxTopLeft(const QPointF & p, double tolerance, const std::pair<double, double> & pixelScale) const;
    bool isNearbyBBoxTopRight(const QPointF & p, double tolerance, const std::pair<double, double> & pixelScale) const;
    bool isNearbyBBoxBtmLeft(const QPointF & p, double tolerance, const std::pair<double, double> & pixelScale) const;
    bool isNearbyBBoxBtmRight(const QPointF & p, double tolerance, const std::pair<double, double> & pixelScale) const;

    bool isNearbyBBoxMidTop(const QPointF & p, double tolerance, const std::pair<double, double> & pixelScale) const;
    bool isNearbyBBoxMidRight(const QPointF & p, double tolerance, const std::pair<double, double> & pixelScale) const;
    bool isNearbyBBoxMidBtm(const QPointF & p, double tolerance, const std::pair<double, double> & pixelScale) const;
    bool isNearbyBBoxMidLeft(const QPointF & p, double tolerance, const std::pair<double, double> & pixelScale) const;

    bool isNearbySelectedCpsBoundingBox(const QPointF & pos, double tolerance) const;

    EventStateEnum isMouseInteractingWithCPSBbox(const QPointF& pos, double tolerance, const std::pair<double, double>& pixelScale) const;

    bool isBboxClickAnywhereEnabled() const;

    void makeStroke(bool prepareForLater, const RotoPoint& p);

    void checkViewersAreDirectlyConnected();

    void showMenuForControlPoint(const BezierCPPtr& cp);

    void showMenuForCurve(const BezierPtr & curve);

    void setCurrentTool(const KnobButtonPtr& tool);

    void onBreakMultiStrokeTriggered();


    /**
     * @brief Set the selection to be the given beziers and the given control points.
     * This can only be called on the main-thread.
     **/
    void setSelection(const std::list<RotoDrawableItemPtr> & selectedBeziers,
                      const std::list<std::pair<BezierCPPtr, BezierCPPtr> > & selectedCps);
    void setSelection(const BezierPtr & curve,
                      const std::pair<BezierCPPtr, BezierCPPtr> & point);

    void getSelection(std::list<RotoDrawableItemPtr>* selectedBeziers,
                      std::list<std::pair<BezierCPPtr, BezierCPPtr> >* selectedCps);

    void setBuiltBezier(const BezierPtr & curve);

    BezierPtr getBezierBeingBuild() const;

    bool smoothSelectedCurve();
    bool cuspSelectedCurve();
    bool removeFeatherForSelectedCurve();
    bool lockSelectedCurves();


    /**
     *@brief Moves of the given pixel the selected control points.
     * This takes into account the zoom factor.
     **/
    bool moveSelectedCpsWithKeyArrows(int x, int y);


    void evaluate(bool redraw);
    void autoSaveAndRedraw();

    void redrawOverlays();


    /**
     * @brief Calls RotoContext::removeItem but also clears some pointers if they point to
     * this curve. For undo/redo purpose.
     **/
    void removeCurve(const RotoDrawableItemPtr& curve);
};

NATRON_NAMESPACE_EXIT

#endif // ROTOPAINTINTERACT_H
