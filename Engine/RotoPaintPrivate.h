/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
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
#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/enable_shared_from_this.hpp>
#endif

#include <QtCore/QPointF>
#include <QtCore/QRectF>


#include <ofxNatron.h>

#include "Engine/BezierCP.h"
#include "Engine/Bezier.h"
#include "Engine/OverlayInteractBase.h"
#ifdef ROTOPAINT_ENABLE_PLANARTRACKER
#include "Engine/PlanarTrackerInteract.h"
#endif
#include "Engine/RotoPaint.h"
#include "Engine/KnobItemsTable.h"
#include "Engine/TransformOverlayInteract.h"
#include "Engine/TrackArgs.h"
#include "Engine/TrackerParamsProvider.h"
#include "Engine/TrackerHelper.h"
#include "Engine/TrackerHelperPrivate.h"
#include "Engine/TrackMarker.h"
#include "Engine/EngineFwd.h"

NATRON_NAMESPACE_ENTER

#ifdef DEBUG
//#define ROTO_PAINT_NODE_GRAPH_VISIBLE
#endif

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

#define kRotoTrackingParamCornerPinPoint1 "cornerPinPoint1"
#define kRotoTrackingParamCornerPinPoint2 "cornerPinPoint2"
#define kRotoTrackingParamCornerPinPoint3 "cornerPinPoint3"
#define kRotoTrackingParamCornerPinPoint4 "cornerPinPoint4"

#define kRotoTrackingParamCornerPinPointLabel1 "Point 1"
#define kRotoTrackingParamCornerPinPointLabel2 "Point 2"
#define kRotoTrackingParamCornerPinPointLabel3 "Point 3"
#define kRotoTrackingParamCornerPinPointLabel4 "Point 4"

#define kRotoTrackingParamOffsetPoint1 "cornerPinOffset1"
#define kRotoTrackingParamOffsetPoint2 "cornerPinOffset2"
#define kRotoTrackingParamOffsetPoint3 "cornerPinOffset3"
#define kRotoTrackingParamOffsetPoint4 "cornerPinOffset4"
#define kRotoTrackingParamOffsetPointLabel1 "Offset 1"
#define kRotoTrackingParamOffsetPointLabel2 "Offset 2"
#define kRotoTrackingParamOffsetPointLabel3 "Offset 3"
#define kRotoTrackingParamOffsetPointLabel4 "Offset 4"
#define kRotoTrackingParamOffsetPointHint "This offset records the modification made by you to the CornerPin points"


#define kRotoTrackingParamReferenceFrame "referenceFrame"
#define kRotoTrackingParamReferenceFrameLabel "Reference Frame"
#define kRotoTrackingParamReferenceFrameHint "This is the frame number at which the CornerPin is an identity"

#define kRotoTrackingParamGoToReferenceFrame "goToReferenceFrame"
#define kRotoTrackingParamGoToReferenceFrameLabel "Go to Reference"
#define kRotoTrackingParamGoToReferenceFrameHint "Move the timeline to the reference frame"


#define kRotoTrackingParamSetToInputRoD "setToInputRod"
#define kRotoTrackingParamSetToInputRoDLabel "Set to Input Rod"
#define kRotoTrackingParamSetToInputRoDHint "Set the 4 CornerPin points to the image rectangle in input of the RotoPaint node"

#define kRotoTrackingParamExportType "exportNodeType"
#define kRotoTrackingParamExportTypeLabel "Node"
#define kRotoTrackingParamExportTypeHint "What kind of node should be created as result of this track"

#define kRotoTrackingParamExportTypeCornerPinMatchMove "CornerPin (relative)"
#define kRotoTrackingParamExportTypeCornerPinMatchMoveHint "Warp the image according to the relative transform between the current frame and the reference frame"

#define kRotoTrackingParamExportTypeCornerPinStabilize "CornerPin (stablize)"
#define kRotoTrackingParamExportTypeCornerPinStabilizeHint "Similar to CornerPin relative but the transform is inversed thus stabilizing the tracked motion"

#define kRotoTrackingParamExportTypeCornerPinTracker "Tracker (copy only)"
#define kRotoTrackingParamExportTypeCornerPinTrackerHint "Exports to a Tracker node with each of the tracks taking 1 corner of the planar surface.\n" \
"This allows you to use the Tracker node's transform functions to stabilize, add jitter, reduce jitter and others.\n" \
"Note that the tracks will contain a copy of the animation of the planar surface and will not be linked."

#define kRotoTrackingParamShowCornerPinOverlay "showCornerPin"
#define kRotoTrackingParamShowCornerPinOverlayLabel "Show CornerPin"
#define kRotoTrackingParamShowCornerPinOverlayHint "If checked the CornerPin will be visible on the viewer and it can be modified. Any modification will create a keyframe: \n" \
"this can be use to contrain the CornerPin to a certain location at a given keyframe"

#define kRotoTrackingParamRefreshViewer "refreshViewer"
#define kRotoTrackingParamRefreshViewerLabel "Refresh Viewer"
#define kRotoTrackingParamRefreshViewerHint "Update viewer during tracking"

#define kRotoTrackingParamCenterViewer "centerViewer"
#define kRotoTrackingParamCenterViewerLabel "Center Viewer"
#define kRotoTrackingParamCenterViewerHint "Center the viewer on the Planar-Track during tracking"

#define kRotoTrackingParamShowPlanarSurfaceGrid "showPlaneGrid"
#define kRotoTrackingParamShowPlanarSurfaceGridLabel "Show Grid"
#define kRotoTrackingParamShowPlanarSurfaceGridHint "Display a grid mapped onto the current plane"

#define kRotoTrackingParamRefreshPlanarTrackTransform "refreshCornerPin"
#define kRotoTrackingParamRefreshPlanarTrackTransformLabel "Refresh CornerPin"
#define kRotoTrackingParamRefreshPlanarTrackTransformHint "Refresh the CornerPin values"

#define kRotoTrackingParamGotoPreviousKeyFrame "goToPrevKeyframe"
#define kRotoTrackingParamGotoPreviousKeyFrameLabel "Previous Key"
#define kRotoTrackingParamGotoPreviousKeyFrameHint "Move the timeline to the previous Planar Surface keyframe"

#define kRotoTrackingParamGotoNextKeyFrame "goToNextKeyframe"
#define kRotoTrackingParamGotoNextKeyFrameLabel "Next Key"
#define kRotoTrackingParamGotoNextKeyFrameHint "Move the timeline to the next Planar Surface keyframe"

#define kRotoTrackingParamSetKeyFrame "setKeyframe"
#define kRotoTrackingParamSetKeyFrameLabel "Set Keyframe"
#define kRotoTrackingParamSetKeyFrameHint "Set a keyframe on the Planar Surface"

#define kRotoTrackingParamRemoveKeyFrame "removeKeyframe"
#define kRotoTrackingParamRemoveKeyFrameLabel "Remove Keyframe"
#define kRotoTrackingParamRemoveKeyFrameHint "Delete the current keyframe on the Planar Surface"


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
#define kRotoUIParamDrawRectangleToolButtonActionLabel "Rectangle Tol"
#define kRotoUIParamDrawRectangleToolButtonActionHint "Draw Rectangle"


#define kRotoUIParamPaintBrushToolButton "PaintBrushToolButton"
#define kRotoUIParamPaintBrushToolButtonLabel "Paint Brush Tool"

#define kRotoUIParamDrawBrushToolButtonAction "PaintSolidTool"
#define kRotoUIParamDrawBrushToolButtonActionLabel "Solid Paint Brush Tool"
#define kRotoUIParamDrawBrushToolButtonActionHint "Draw using the pen"

#define kRotoUIParamPencilToolButtonAction "PencilTool"
#define kRotoUIParamPencilToolButtonActionLabel "Pencil Tool"
#define kRotoUIParamPencilToolButtonActionHint "Draw open Bezier"

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

#define kRotoUIParamRightClickMenuActionCreatePlanarTrack "createPlanarTrack"
#define kRotoUIParamRightClickMenuActionCreatePlanarTrackLabel "Create Planar-Track"
#define kRotoUIParamRightClickMenuActionCreatePlanarTrackHint "Create a Planar-Track group from the selected shape(s)"

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
#define kRotoUIParamRippleEditHint "Ripple-edit: When activated, moving a control point" \
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
#define kRotoUIParamSourceTypeHint "Source color used for painting the stroke when the Reveal/Clone tools are used."
#define kRotoUIParamSourceTypeOptionForegroundHint "The painted result at this point in the hierarchy."
#define kRotoUIParamSourceTypeOptionBackgroundHint "The original image unpainted connected to bg."
#define kRotoUIParamSourceTypeOptionBackgroundNHint "The original image unpainted connected to bg%1."

#define kRotoUIParamResetCloneOffset "resetCloneOffsetButton"
#define kRotoUIParamResetCloneOffsetLabel "Reset Transform"
#define kRotoUIParamResetCloneOffsetHint "Reset the transform applied before cloning to identity"

#define kRotoUIParamMultiStrokeEnabled "multiStrokeEnabledButton"
#define kRotoUIParamMultiStrokeEnabledLabel "Multi-Stroke"
#define kRotoUIParamMultiStrokeEnabledHint "When checked, strokes will be appended to the same item " \
    "in the hierarchy as long as the same tool is selected.\n" \
    "Select another tool to make a new item."


// Shortcuts

#define kShortcutActionRotoDelete "delete"
#define kShortcutActionRotoDeleteLabel "Delete Element"

#define kShortcutActionRotoCloseBezier "closeBezier"
#define kShortcutActionRotoCloseBezierLabel "Close Bezier"

#define kShortcutActionRotoSelectAll "selectAll"
#define kShortcutActionRotoSelectAllLabel "Select All"

#define kShortcutActionRotoSelectionTool "selectionTool"
#define kShortcutActionRotoSelectionToolLabel "Switch to Selection Mode"

#define kShortcutActionRotoAddTool "addTool"
#define kShortcutActionRotoAddToolLabel "Switch to Add Mode"

#define kShortcutActionRotoEditTool "editTool"
#define kShortcutActionRotoEditToolLabel "Switch to Edition Mode"

#define kShortcutActionRotoBrushTool "brushTool"
#define kShortcutActionRotoBrushToolLabel "Switch to Brush Mode"

#define kShortcutActionRotoCloneTool "cloneTool"
#define kShortcutActionRotoCloneToolLabel "Switch to Clone Mode"

#define kShortcutActionRotoEffectTool "EffectTool"
#define kShortcutActionRotoEffectToolLabel "Switch to Effect Mode"

#define kShortcutActionRotoColorTool "colorTool"
#define kShortcutActionRotoColorToolLabel "Switch to Color Mode"

#define kShortcutActionRotoNudgeLeft "nudgeLeft"
#define kShortcutActionRotoNudgeLeftLabel "Move Bezier to the Left"

#define kShortcutActionRotoNudgeRight "nudgeRight"
#define kShortcutActionRotoNudgeRightLabel "Move Bezier to the Right"

#define kShortcutActionRotoNudgeBottom "nudgeBottom"
#define kShortcutActionRotoNudgeBottomLabel "Move Bezier to the Bottom"

#define kShortcutActionRotoNudgeTop "nudgeTop"
#define kShortcutActionRotoNudgeTopLabel "Move Bezier to the Top"

#define kShortcutActionRotoSmooth "smooth"
#define kShortcutActionRotoSmoothLabel "Smooth Bezier"

#define kShortcutActionRotoCuspBezier "cusp"
#define kShortcutActionRotoCuspBezierLabel "Cusp Bezier"

#define kShortcutActionRotoRemoveFeather "rmvFeather"
#define kShortcutActionRotoRemoveFeatherLabel "Remove Feather"

#define kShortcutActionRotoLockCurve "lock"
#define kShortcutActionRotoLockCurveLabel "Lock Shape"


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


class RotoPaintKnobItemsTable : public KnobItemsTable
{

public:

    RotoPaintPrivate* _imp;

    RotoPaintKnobItemsTable(RotoPaintPrivate* imp,
                            KnobItemsTableTypeEnum type);

    virtual ~RotoPaintKnobItemsTable()
    {

    }

    virtual std::string getTableIdentifier() const OVERRIDE FINAL
    {
        return kRotoPaintItemsTableName;
    }

    virtual std::string getTablePythonPrefix() const OVERRIDE FINAL
    {
        return "roto";
    }

    virtual KnobTableItemPtr createItemFromSerialization(const SERIALIZATION_NAMESPACE::KnobTableItemSerializationPtr& data) OVERRIDE FINAL;
    
    virtual SERIALIZATION_NAMESPACE::KnobTableItemSerializationPtr createSerializationFromItem(const KnobTableItemPtr& item) OVERRIDE FINAL;

    virtual void fromSerialization(const SERIALIZATION_NAMESPACE::SerializationObjectBase & obj) OVERRIDE FINAL;

    virtual void onModelReset() OVERRIDE FINAL;

    /**
     * @brief Returns a list of all the curves in the order in which they should be rendered.
     * Non-active curves will not be inserted into the list.
     * MT-safe
     **/
    std::list<RotoDrawableItemPtr> getRotoPaintItemsByRenderOrder() const;
    std::list<RotoDrawableItemPtr> getActivatedRotoPaintItemsByRenderOrder(TimeValue time, ViewIdx view) const;

    SelectedItems getSelectedDrawableItems() const;

#ifdef ROTOPAINT_ENABLE_PLANARTRACKER
    PlanarTrackLayerPtr getSelectedPlanarTrack(bool alsoLookForParentIfShapeSelected) const;
#endif
};


class RotoPaintInteract;
class RotoPaintPrivate
#ifdef ROTOPAINT_ENABLE_PLANARTRACKER
: public TrackerParamsProvider
#endif
{
public:

    RotoPaint* publicInterface; // can not be a smart ptr
    RotoPaint::RotoPaintTypeEnum nodeType;
    KnobBoolWPtr premultKnob;
    KnobChoiceWPtr outputComponentsKnob;
    KnobBoolWPtr clipToFormatKnob;
    KnobChoiceWPtr outputRoDTypeKnob;
    KnobChoiceWPtr outputFormatKnob;
    KnobIntWPtr outputFormatSizeKnob;
    KnobDoubleWPtr outputFormatParKnob;
    KnobKeyFrameMarkersWPtr shapeKeysKnob;
    KnobBoolWPtr enabledKnobs[4];

    RotoPaintInteractPtr ui;
#ifdef ROTOPAINT_ENABLE_PLANARTRACKER
    boost::shared_ptr<PlanarTrackerInteract> planarTrackInteract;
#endif
    boost::shared_ptr<TransformOverlayInteract> transformInteract, cloneTransformInteract;

    // The group internal input nodes
    std::vector<NodeWPtr> inputNodes;
    NodeWPtr premultNode;

    boost::shared_ptr<RotoPaintKnobItemsTable> knobsTable;

    // Merge node (or more if there are more than 64 items) used when all items share the same compositing operator to make the rotopaint tree shallow
    mutable QMutex globalMergeNodesMutex;
    NodesList globalMergeNodes;
    NodePtr globalTimeBlurNode;

    // The temporary solo items
    mutable QMutex soloItemsMutex;
    std::set<RotoDrawableItemWPtr> soloItems;

    TrackerHelperPtr tracker;

    // Set if we are currently tracking
#ifdef ROTOPAINT_ENABLE_PLANARTRACKER
    PlanarTrackLayerPtr activePlanarTrack;
#endif

    // Recursive counter to prevent refreshing of the node tree
    int treeRefreshBlocked;

    KnobBoolWPtr customRangeKnob;
    KnobChoiceWPtr lifeTimeKnob;
    KnobIntWPtr lifeTimeFrameKnob;

    KnobButtonWPtr resetCenterKnob;
    KnobButtonWPtr resetCloneCenterKnob;
    KnobButtonWPtr resetCloneTransformKnob;
    KnobButtonWPtr resetTransformKnob;

    KnobChoiceWPtr motionBlurTypeKnob;
    KnobIntWPtr motionBlurKnob, globalMotionBlurKnob;
    KnobDoubleWPtr shutterKnob, globalShutterKnob;
    KnobChoiceWPtr shutterTypeKnob, globalShutterTypeKnob;
    KnobDoubleWPtr customOffsetKnob, globalCustomOffsetKnob;

    KnobChoiceWPtr mergeInputAChoiceKnob;
    KnobDoubleWPtr cloneTranslateKnob;
    KnobDoubleWPtr cloneRotateKnob;
    KnobDoubleWPtr cloneScaleKnob;
    KnobBoolWPtr cloneUniformKnob;
    KnobDoubleWPtr cloneSkewXKnob;
    KnobDoubleWPtr cloneSkewYKnob;
    KnobChoiceWPtr cloneSkewOrderKnob;
    KnobDoubleWPtr cloneCenterKnob;
    
    KnobDoubleWPtr translateKnob;
    KnobDoubleWPtr rotateKnob;
    KnobDoubleWPtr scaleKnob;
    KnobBoolWPtr scaleUniformKnob;
    KnobDoubleWPtr skewXKnob;
    KnobDoubleWPtr skewYKnob;
    KnobChoiceWPtr skewOrderKnob;
    KnobDoubleWPtr extraMatrixKnob;
    KnobDoubleWPtr centerKnob;

    KnobDoubleWPtr mixKnob;
    KnobButtonWPtr addGroupButtonKnob, addLayerButtonKnob, removeItemButtonKnob;

    KnobPageWPtr trackingPage;
    KnobBoolWPtr enableTrackRed, enableTrackGreen, enableTrackBlue;
    KnobDoubleWPtr maxError;
    KnobIntWPtr maxIterations;
    KnobBoolWPtr bruteForcePreTrack, useNormalizedIntensities;
    KnobDoubleWPtr preBlurSigma;
    KnobChoiceWPtr motionModel;
    KnobIntWPtr trackerReferenceFrame;
    KnobButtonWPtr goToReferenceFrameKnob;
    KnobButtonWPtr setReferenceFrameToCurrentFrameKnob;

    KnobGroupWPtr cornerPinGroup;
    KnobDoubleWPtr offsetPoints[4], toPoints[4];
    KnobButtonWPtr setFromPointsToInputRodButton;

    KnobButtonWPtr showCornerPinOverlay;
    KnobButtonWPtr showPlaneGrid;

    KnobButtonWPtr refreshCornerPinButton;


    KnobSeparatorWPtr exportDataSep;
    KnobChoiceWPtr exportType;
    KnobBoolWPtr exportLink;
    KnobButtonWPtr exportButton;

    KnobButtonWPtr trackRangeButton;
    KnobButtonWPtr trackBwButton;
    KnobButtonWPtr trackPrevButton;
    KnobButtonWPtr stopTrackingButton; //< invisible
    KnobButtonWPtr trackNextButton;
    KnobButtonWPtr trackFwButton;
    KnobButtonWPtr clearAllAnimationButton;
    KnobButtonWPtr clearBwAnimationButton;
    KnobButtonWPtr clearFwAnimationButton;
    KnobButtonWPtr updateViewerButton;
    KnobButtonWPtr centerViewerButton;

    KnobButtonWPtr goToPreviousCornerPinKeyframeButton;
    KnobButtonWPtr goToNextCornerPinKeyframeButton;
    KnobButtonWPtr setCornerPinKeyframeButton;
    KnobButtonWPtr removeCornerPinKeyframeButton;

    // Track range dialog
    KnobGroupWPtr trackRangeDialogGroup;
    KnobIntWPtr trackRangeDialogFirstFrame;
    KnobIntWPtr trackRangeDialogLastFrame;
    KnobIntWPtr trackRangeDialogStep;
    KnobButtonWPtr trackRangeDialogOkButton;
    KnobButtonWPtr trackRangeDialogCancelButton;

    
    RotoPaintPrivate(RotoPaint* publicInterface,
                     RotoPaint::RotoPaintTypeEnum type);


#ifdef ROTOPAINT_ENABLE_PLANARTRACKER
    //////////////////// Overriden from TrackerParamsProvider
    virtual bool trackStepFunctor(int trackIndex, const TrackArgsBasePtr& args, int frame) OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual NodePtr getTrackerNode() const OVERRIDE FINAL;
    virtual NodePtr getSourceImageNode() const OVERRIDE FINAL;
    virtual ImagePlaneDesc getMaskImagePlane(int *channelIndex) const OVERRIDE FINAL;
    virtual NodePtr getMaskImageNode() const OVERRIDE FINAL;
    virtual TrackerHelperPtr getTracker() const OVERRIDE FINAL;
    virtual bool getCenterOnTrack() const OVERRIDE FINAL;
    virtual bool getUpdateViewer() const OVERRIDE FINAL;
    virtual void getTrackChannels(bool* doRed, bool* doGreen, bool* doBlue) const OVERRIDE FINAL;
    virtual bool canDisableMarkersAutomatically() const OVERRIDE FINAL;
    virtual double getMaxError() const OVERRIDE FINAL;
    virtual int getMaxNIterations() const OVERRIDE FINAL;
    virtual bool isBruteForcePreTrackEnabled() const OVERRIDE FINAL;
    virtual bool isNormalizeIntensitiesEnabled() const OVERRIDE FINAL;
    virtual double getPreBlurSigma() const OVERRIDE FINAL;
    virtual RectD getNormalizationRoD(TimeValue time, ViewIdx view) const OVERRIDE FINAL;
    ////////////////////

    RectD getInputRoD(TimeValue time, ViewIdx view) const;

    void setFromPointsToInputRod();
#endif

    NodePtr getOrCreateGlobalMergeNode(int blendingOperator, int *availableInputIndex);

    NodePtr getOrCreateGlobalTimeBlurNode();

    bool isRotoPaintTreeConcatenatableInternal(const std::list<RotoDrawableItemPtr>& items,
                                               int* blendingMode) const;

    void exportTrackDataFromExportOptions();

    void refreshMotionBlurKnobsVisibility();


    void resetTransformCenter();

    void resetCloneTransformCenter();

    void resetTransformsCenter(bool doClone, bool doTransform);

    void resetTransform();

    void resetCloneTransform();

    void resetTransformInternal(const KnobDoublePtr& translate,
                                const KnobDoublePtr& scale,
                                const KnobDoublePtr& center,
                                const KnobDoublePtr& rotate,
                                const KnobDoublePtr& skewX,
                                const KnobDoublePtr& skewY,
                                const KnobBoolPtr& scaleUniform,
                                const KnobChoicePtr& skewOrder,
                                const KnobDoublePtr& extraMatrix);

    void refreshFormatVisibility();

    void refreshSourceKnobs();

    void createBaseLayer();

    void connectRotoPaintBottomTreeToItems(bool canConcatenate,
                                           const RotoPaintPtr& rotoPaintEffect,
                                           const NodePtr& premultNode,
                                           const NodePtr& globalTimeBlurNode,
                                           const NodePtr& treeOutputNode,
                                           const NodePtr& mergeNode);

    void refreshRegisteredOverlays();

#ifdef ROTOPAINT_ENABLE_PLANARTRACKER
    void onTrackRangeClicked();

    void onTrackRangeOkClicked();

    void onTrackBwClicked();

    void onTrackPrevClicked();

    void onStopButtonClicked();

    void onTrackNextClicked();

    void onTrackFwClicked();

    void onSetReferenceFrameClicked();

    void onClearAllAnimationClicked();

    void onClearBwAnimationClicked();

    void onClearFwAnimationClicked();

    void onGotoReferenceFrameButtonClicked();

    void onGotoPreviousCornerPinKeyframeButtonClicked();
    void onGotoNextCornerPinKeyframeButtonClicked();

    void onSetCornerPinKeyframeButtonClicked();
    void onRemoveCornerPinKeyframeButtonClicked();

    void onCornerPinPointChangedByUser(TimeValue time, int index);

    TrackMarkerPtr createTrackForTracking(TimeValue startingFrame, const std::list<RotoDrawableItemPtr>& shapes);

    void updateMatrixFromTrack(const TrackMarkerPtr& track, TimeValue time);

    void updateCornerPinFromMatrixAtAllTracks(const PlanarTrackLayerPtr& planarTrack);
    void updateCornerPinFromMatrixAtAllTrack_threaded();

    void updateCornerPinFromMatrixAtTime(TimeValue time, const Transform::Matrix3x3& transform, const PlanarTrackLayerPtr& planarTrack);

    void updateMatrixFromCornerPinAtTime(TimeValue time, const PlanarTrackLayerPtr& planarTrack);
    void updateMatrixFromCornerPinForAllKeyframes(const PlanarTrackLayerPtr& planarTrack);
    void updateMatrixFromCornerPinForAllKeyframes_threaded();

    void createPlanarTrackForSelectedShapes();

    void refreshTrackingControlsVisiblity();

    void refreshTrackingControlsEnabledness();


    void startTrackingInternal(TimeValue startFrame, TimeValue lastFrame, TimeValue step, const std::list<RotoDrawableItemPtr>& shapes, OverlaySupport* overlay);

    static void getTransformFromPoints(Point fromPoints[4], Point toPoints[4], Transform::Matrix3x3* transform);

    void getTransformFromCornerPinParamsAtTime(TimeValue time, const PlanarTrackLayerPtr& planarTrack, Transform::Matrix3x3* transform);
#endif // #ifdef ROTOPAINT_ENABLE_PLANARTRACKER
};


class RotoPaintInteract
: public boost::enable_shared_from_this<RotoPaintInteract>
, public OverlayInteractBase
{
public:
    RotoPaintPrivate* _imp;
    SelectedCPs selectedCps;
    QRectF selectedCpsBbox;
    bool showCpsBbox;

    ////This is by default eSelectedCpsTransformModeTranslateAndScale. When clicking the cross-hair in the center this will toggle the transform mode
    ////like it does in inkscape.
    SelectedCpsTransformModeEnum transformMode;
    BezierPtr builtBezier; //< the Bezier currently being built
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
    KnobButtonWPtr createPlanarTrackAction;

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
    // constructors should be privatized in any class that derives from boost::enable_shared_from_this<>

    RotoPaintInteract(RotoPaintPrivate* p);

public:
    static RotoPaintInteractPtr create(RotoPaintPrivate* p) WARN_UNUSED_RETURN
    {
        return RotoPaintInteractPtr( new RotoPaintInteract(p) );
    }

    virtual void drawOverlay(TimeValue time, const RenderScale & renderScale, ViewIdx view) OVERRIDE FINAL;
    virtual bool onOverlayPenDown(TimeValue time, const RenderScale & renderScale, ViewIdx view, const QPointF & viewportPos, const QPointF & pos, double pressure, TimeValue timestamp, PenType pen) OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual bool onOverlayPenMotion(TimeValue time, const RenderScale & renderScale, ViewIdx view,
                                const QPointF & viewportPos, const QPointF & pos, double pressure, TimeValue timestamp) OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual bool onOverlayPenUp(TimeValue time, const RenderScale & renderScale, ViewIdx view, const QPointF & viewportPos, const QPointF & pos, double pressure, TimeValue timestamp) OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual bool onOverlayPenDoubleClicked(TimeValue time,
                                       const RenderScale & renderScale,
                                       ViewIdx view,
                                       const QPointF & viewportPos,
                                       const QPointF & pos) OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual bool onOverlayKeyDown(TimeValue time, const RenderScale & renderScale, ViewIdx view, Key key, KeyboardModifiers modifiers) OVERRIDE FINAL;
    virtual bool onOverlayKeyUp(TimeValue time, const RenderScale & renderScale, ViewIdx view, Key key, KeyboardModifiers modifiers) OVERRIDE FINAL;
    virtual bool onOverlayKeyRepeat(TimeValue time, const RenderScale & renderScale, ViewIdx view, Key key, KeyboardModifiers modifiers) OVERRIDE FINAL;
    virtual bool onOverlayFocusGained(TimeValue time, const RenderScale & renderScale, ViewIdx view) OVERRIDE FINAL;
    virtual bool onOverlayFocusLost(TimeValue time, const RenderScale & renderScale, ViewIdx view) OVERRIDE FINAL;

    virtual void onViewportSelectionCleared() OVERRIDE FINAL;

    virtual void onViewportSelectionUpdated(const RectD& rectangle, bool onRelease) OVERRIDE FINAL;

    bool isFeatherVisible() const;

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

    void removeItemFromSelection(const RotoDrawableItemPtr& b);

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

    void drawSelectedCp(TimeValue time,
                        const BezierCPPtr & cp,
                        double x, double y,
                        const Transform::Matrix3x3& transform);

    std::pair<BezierCPPtr, BezierCPPtr>isNearbyFeatherBar(TimeValue time, ViewIdx view, const std::pair<double, double> & pixelScale, const QPointF & pos) const;

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


    /**
    * @brief Returns a Bezier curves nearby the point (x,y) and the parametric value
    * which would be used to find the exact Bezier point lying on the curve.
    **/
    BezierPtr isNearbyBezier(double x, double y, TimeValue time, ViewIdx view, double acceptance, int* index, double* t, bool *feather) const;

    bool isNearbySelectedCpsBoundingBox(const QPointF & pos, double tolerance) const;

    EventStateEnum isMouseInteractingWithCPSBbox(const QPointF& pos, double tolerance, const std::pair<double, double>& pixelScale) const;

    bool isBboxClickAnywhereEnabled() const;

    void makeStroke(bool prepareForLater, const RotoPoint& p);

    void showMenuForControlPoint(const BezierCPPtr& cp);

    void showMenuForCurve(const BezierPtr & curve);

    void setCurrentTool(const KnobButtonPtr& tool);

    void onBreakMultiStrokeTriggered();


    /**
     * @brief Set the selection to be the given Beziers and the given control points.
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

    bool smoothSelectedCurve(TimeValue time, ViewIdx view);
    bool cuspSelectedCurve(TimeValue time, ViewIdx view);
    bool removeFeatherForSelectedCurve(ViewIdx view);
    bool lockSelectedCurves();


    /**
     *@brief Moves of the given pixel the selected control points.
     * This takes into account the zoom factor.
     **/
    bool moveSelectedCpsWithKeyArrows(int x, int y, TimeValue time, ViewIdx view);

    void autoSaveAndRedraw();


    /**
     * @brief Calls RotoContext::removeItem but also clears some pointers if they point to
     * this curve. For undo/redo purpose.
     **/
    void removeCurve(const RotoDrawableItemPtr& curve);

};

NATRON_NAMESPACE_EXIT

#endif // ROTOPAINTINTERACT_H
