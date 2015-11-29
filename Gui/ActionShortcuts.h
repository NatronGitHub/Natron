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

#ifndef ACTIONSHORTCUTS_H
#define ACTIONSHORTCUTS_H

/**
 * @brief In this file all Natron's actions that can have their shortcut edited should be listed.
 **/

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include <map>
#include <list>
#include <vector>

#include "Global/Macros.h"
CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QKeyEvent>
#include <QMouseEvent>
#include <QString>
#include <QAction>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#define kShortcutGroupGlobal "Global"
#define kShortcutGroupNodegraph "NodeGraph"
#define kShortcutGroupCurveEditor "CurveEditor"
#define kShortcutGroupDopeSheetEditor "DopeSheetEditor"
#define kShortcutGroupViewer "Viewer"
#define kShortcutGroupRoto "Roto"
#define kShortcutGroupTracking "Tracking"
#define kShortcutGroupPlayer "Player"
#define kShortcutGroupNodes "Nodes"
#define kShortcutGroupScriptEditor "ScriptEditor"

/////////GLOBAL SHORTCUTS
#define kShortcutIDActionNewProject "newProject"
#define kShortcutDescActionNewProject "New Project"

#define kShortcutIDActionOpenProject "openProject"
#define kShortcutDescActionOpenProject "Open Project..."

#define kShortcutIDActionCloseProject "closeProject"
#define kShortcutDescActionCloseProject "Close Project"

#define kShortcutIDActionSaveProject "saveProject"
#define kShortcutDescActionSaveProject "Save Project"

#define kShortcutIDActionSaveAsProject "saveAsProject"
#define kShortcutDescActionSaveAsProject "Save Project As.."

#define kShortcutIDActionSaveAndIncrVersion "saveAndIncr"
#define kShortcutDescActionSaveAndIncrVersion "New Project Version"

#define kShortcutIDActionExportProject "exportAsGroup"
#define kShortcutDescActionExportProject "Export Project As Group"

#define kShortcutIDActionPreferences "preferences"
#define kShortcutDescActionPreferences "Preferences..."

#define kShortcutIDActionQuit "quit"
#define kShortcutDescActionQuit "Quit"

#define kShortcutIDActionProjectSettings "projectSettings"
#define kShortcutDescActionProjectSettings "Show Project Settings..."

#define kShortcutIDActionShowOFXLog "showOFXLog"
#define kShortcutDescActionShowOFXLog "Show Project Errors Log..."

#define kShortcutIDActionShowShortcutEditor "showShortcutEditor"
#define kShortcutDescActionShowShortcutEditor "Show Shortcuts Editor..."

#define kShortcutIDActionNewViewer "newViewer"
#define kShortcutDescActionNewViewer "New Viewer"

#define kShortcutIDActionFullscreen "fullScreen"
#define kShortcutDescActionFullscreen "Enter Full Screen"

#define kShortcutIDActionShowWindowsConsole "showApplicationConsole"
#define kShortcutDescActionShowWindowsConsole "Show/Hide Application Console"

#define kShortcutIDActionClearDiskCache "clearDiskCache"
#define kShortcutDescActionClearDiskCache "Clear Disk Cache"

#define kShortcutIDActionClearPlaybackCache "clearPlaybackCache"
#define kShortcutDescActionClearPlaybackCache "Clear Playback Cache"

#define kShortcutIDActionClearNodeCache "clearNodeCache"
#define kShortcutDescActionClearNodeCache "Clear Per-Node Cache"

#define kShortcutIDActionClearPluginsLoadCache "clearPluginsCache"
#define kShortcutDescActionClearPluginsLoadCache "Clear Plug-ins Load Cache"

#define kShortcutIDActionClearAllCaches "clearAllCaches"
#define kShortcutDescActionClearAllCaches "Clear All Caches"

#define kShortcutIDActionShowAbout "showAbout"
#define kShortcutDescActionShowAbout "About..."

#define kShortcutIDActionRenderSelected "renderSelect"
#define kShortcutDescActionRenderSelected "Render Selected Writers"

#define kShortcutIDActionEnableRenderStats "enableRenderStats"
#define kShortcutDescActionEnableRenderStats "Enable render statistics"

#define kShortcutIDActionRenderAll "renderAll"
#define kShortcutDescActionRenderAll "Render All Writers"

#define kShortcutIDActionConnectViewerToInput1 "connectViewerInput1"
#define kShortcutDescActionConnectViewerToInput1 "Connect Viewer to Input 1"

#define kShortcutIDActionConnectViewerToInput2 "connectViewerInput2"
#define kShortcutDescActionConnectViewerToInput2 "Connect Viewer to Input 2"

#define kShortcutIDActionConnectViewerToInput3 "connectViewerInput3"
#define kShortcutDescActionConnectViewerToInput3 "Connect Viewer to Input 3"

#define kShortcutIDActionConnectViewerToInput4 "connectViewerInput4"
#define kShortcutDescActionConnectViewerToInput4 "Connect Viewer to Input 4"

#define kShortcutIDActionConnectViewerToInput5 "connectViewerInput5"
#define kShortcutDescActionConnectViewerToInput5 "Connect Viewer to Input 5"

#define kShortcutIDActionConnectViewerToInput6 "connectViewerInput6"
#define kShortcutDescActionConnectViewerToInput6 "Connect Viewer to Input 6"

#define kShortcutIDActionConnectViewerToInput7 "connectViewerInput7"
#define kShortcutDescActionConnectViewerToInput7 "Connect Viewer to Input 7"

#define kShortcutIDActionConnectViewerToInput8 "connectViewerInput8"
#define kShortcutDescActionConnectViewerToInput8 "Connect Viewer to Input 8"

#define kShortcutIDActionConnectViewerToInput9 "connectViewerInput9"
#define kShortcutDescActionConnectViewerToInput9 "Connect Viewer to Input 9"

#define kShortcutIDActionConnectViewerToInput10 "connectViewerInput10"
#define kShortcutDescActionConnectViewerToInput10 "Connect Viewer to Input 10"


#define kShortcutIDActionShowPaneFullScreen "showPaneFullScreen"
#define kShortcutDescActionShowPaneFullScreen "Show Pane Full Screen"

#define kShortcutIDActionImportLayout "importLayout"
#define kShortcutDescActionImportLayout "Import Layout..."

#define kShortcutIDActionExportLayout "exportLayout"
#define kShortcutDescActionExportLayout "Export Layout..."

#define kShortcutIDActionDefaultLayout "restoreDefaultLayout"
#define kShortcutDescActionDefaultLayout "Restore Default Layout"

#define kShortcutIDActionNextTab "nextTab"
#define kShortcutDescActionNextTab "Next Tab"

#define kShortcutIDActionPrevTab "prevTab"
#define kShortcutDescActionPrevTab "Previous Tab"

#define kShortcutIDActionCloseTab "closeTab"
#define kShortcutDescActionCloseTab "Close Tab"

/////////VIEWER SHORTCUTS
#define kShortcutIDActionLuminance "luminance"
#define kShortcutDescActionLuminance "Display Luminance"

#define kShortcutIDActionRed "channelR"
#define kShortcutDescActionRed "Display Red Channel"

#define kShortcutIDActionGreen "channelG"
#define kShortcutDescActionGreen "Display Green Channel"

#define kShortcutIDActionBlue "channelB"
#define kShortcutDescActionBlue "Display Blue Channel"

#define kShortcutIDActionAlpha "channelA"
#define kShortcutDescActionAlpha "Display Alpha Channel"

#define kShortcutIDActionLuminanceA "luminanceA"
#define kShortcutDescActionLuminanceA "Display Luminance on input A only"

#define kShortcutIDActionMatteOverlay "matteOverlay"
#define kShortcutDescActionMatteOverlay "Overlay the channel alpha channel"

#define kShortcutIDActionRedA "channelRA"
#define kShortcutDescActionRedA "Display Red Channel on input A only"

#define kShortcutIDActionGreenA "channelGA"
#define kShortcutDescActionGreenA "Display Green Channel on input A only"

#define kShortcutIDActionBlueA "channelBA"
#define kShortcutDescActionBlueA "Display Blue Channel on input A only"

#define kShortcutIDActionAlphaA "channelAA"
#define kShortcutDescActionAlphaA "Display Alpha Channel on input A only"

#define kShortcutIDActionFitViewer "fitViewer"
#define kShortcutDescActionFitViewer "Fit Image to Viewer"

#define kShortcutIDActionClipEnabled "clipEnabled"
#define kShortcutDescActionClipEnabled "Enable Clipping to Project Window"

#define kShortcutIDActionRefresh "refresh"
#define kShortcutDescActionRefresh "Refresh Image"

#define kShortcutIDActionRefreshWithStats "refreshWithStats"
#define kShortcutDescActionRefreshWithStats "Refresh Image and show render statistics"

#define kShortcutIDActionROIEnabled "userRoiEnabled"
#define kShortcutDescActionROIEnabled "Enable User RoI"

#define kShortcutIDActionNewROI "newRoi"
#define kShortcutDescActionNewROI "New user RoI"


#define kShortcutIDActionProxyEnabled "proxyEnabled"
#define kShortcutDescActionProxyEnabled "Enable Proxy Rendering"

#define kShortcutIDActionProxyLevel2 "proxy2"
#define kShortcutDescActionProxyLevel2 "Proxy Level 2"

#define kShortcutIDActionProxyLevel4 "proxy4"
#define kShortcutDescActionProxyLevel4 "Proxy Level 4"

#define kShortcutIDActionProxyLevel8 "proxy8"
#define kShortcutDescActionProxyLevel8 "Proxy Level 8"

#define kShortcutIDActionProxyLevel16 "proxy16"
#define kShortcutDescActionProxyLevel16 "Proxy Level 16"

#define kShortcutIDActionProxyLevel32 "proxy32"
#define kShortcutDescActionProxyLevel32 "Proxy Level 32"

#define kShortcutIDActionZoomLevel100 "zoom100"
#define kShortcutDescActionZoomLevel100 "Set Zoom to 100%"

#define kShortcutIDActionZoomIn "zoomIn"
#define kShortcutDescActionZoomIn "Zoom In"

#define kShortcutIDActionZoomOut "zoomOut"
#define kShortcutDescActionZoomOut "Zoom Out"

#define kShortcutIDActionHideOverlays "hideOverlays"
#define kShortcutDescActionHideOverlays "Show/Hide Overlays"

#define kShortcutIDActionHidePlayer "hidePlayer"
#define kShortcutDescActionHidePlayer "Show/Hide Player"

#define kShortcutIDActionHideTimeline "hideTimeline"
#define kShortcutDescActionHideTimeline "Show/Hide Timeline"

#define kShortcutIDActionHideLeft "hideLeft"
#define kShortcutDescActionHideLeft "Show/Hide Left Toolbar"

#define kShortcutIDActionHideRight "hideRight"
#define kShortcutDescActionHideRight "Show/Hide Right Toolbar"

#define kShortcutIDActionHideTop "hideTop"
#define kShortcutDescActionHideTop "Show/Hide Top Toolbar"

#define kShortcutIDActionHideInfobar "hideInfo"
#define kShortcutDescActionHideInfobar "Show/Hide info bar"

#define kShortcutIDActionHideAll "hideAll"
#define kShortcutDescActionHideAll "Hide All"

#define kShortcutIDActionShowAll "showAll"
#define kShortcutDescActionShowAll "Show All"

#define kShortcutIDMousePickColor "pick"
#define kShortcutDescMousePickColor "Pick a Color"

#define kShortcutIDMouseRectanglePick "rectanglePick"
#define kShortcutDescMouseRectanglePick "Rectangle Color Picker"

#define kShortcutIDToggleWipe "toggleWipe"
#define kShortcutDescToggleWipe "Toggle Wipe"

#define kShortcutIDCenterWipe "centerWipe"
#define kShortcutDescCenterWipe "Center Wipe on Mouse"

#define kShortcutIDNextLayer "nextLayer"
#define kShortcutDescNextLayer "Next Layer"

#define kShortcutIDPrevLayer "prevLayer"
#define kShortcutDescPrevLayer "Previous Layer"

#define kShortcutIDSwitchInputAAndB "switchAB"
#define kShortcutDescSwitchInputAAndB "Switch Input A and B"

#define kShortcutIDShowLeftView "leftView"
#define kShortcutDescShowLeftView "Left"

#define kShortcutIDShowRightView "rightView"
#define kShortcutDescShowRightView "Right"

///////////PLAYER SHORTCUTS

#define kShortcutIDActionPlayerPrevious "prev"
#define kShortcutDescActionPlayerPrevious "Previous Frame"

#define kShortcutIDActionPlayerNext "next"
#define kShortcutDescActionPlayerNext "Next Frame"

#define kShortcutIDActionPlayerBackward "backward"
#define kShortcutDescActionPlayerBackward "Play Backward"

#define kShortcutIDActionPlayerForward "forward"
#define kShortcutDescActionPlayerForward "Play Forward"

#define kShortcutIDActionPlayerStop "stop"
#define kShortcutDescActionPlayerStop "Stop"

#define kShortcutIDActionPlayerPrevIncr "prevIncr"
#define kShortcutDescActionPlayerPrevIncr "Go to Current Frame Minus Increment"

#define kShortcutIDActionPlayerNextIncr "nextIncr"
#define kShortcutDescActionPlayerNextIncr "Go to Current Frame Plus Increment"

#define kShortcutIDActionPlayerPrevKF "prevKF"
#define kShortcutDescActionPlayerPrevKF "Go to Previous Keyframe"

#define kShortcutIDActionPlayerNextKF "nextKF"
#define kShortcutDescActionPlayerNextKF "Go to Next Keyframe"

#define kShortcutIDActionPlayerFirst "first"
#define kShortcutDescActionPlayerFirst "Go to First Frame"

#define kShortcutIDActionPlayerLast "last"
#define kShortcutDescActionPlayerLast "Go to Last Frame"


///////////ROTO SHORTCUTS
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

#define kShortcutIDActionRotoLinkToTrack "linkToTrack"
#define kShortcutDescActionRotoLinkToTrack "Link to Track"

#define kShortcutIDActionRotoUnlinkToTrack "unlinkFromTrack"
#define kShortcutDescActionRotoUnlinkToTrack "Unlink from Track"

#define kShortcutIDActionRotoLockCurve "lock"
#define kShortcutDescActionRotoLockCurve "Lock Shape"

///////////TRACKING SHORTCUTS
#define kShortcutIDActionTrackingSelectAll "selectAll"
#define kShortcutDescActionTrackingSelectAll "Select All Tracks"

#define kShortcutIDActionTrackingDelete "delete"
#define kShortcutDescActionTrackingDelete "Remove Tracks"

#define kShortcutIDActionTrackingBackward "backward"
#define kShortcutDescActionTrackingBackward "Track Backward"

#define kShortcutIDActionTrackingForward "forward"
#define kShortcutDescActionTrackingForward "Track Forward"

#define kShortcutIDActionTrackingPrevious "prev"
#define kShortcutDescActionTrackingPrevious "Track to Previous Frame"

#define kShortcutIDActionTrackingNext "next"
#define kShortcutDescActionTrackingNext "Track to Next Frame"

#define kShortcutIDActionTrackingStop "stop"
#define kShortcutDescActionTrackingStop "Stop Tracking"

///////////NODEGRAPH SHORTCUTS
#define kShortcutIDActionGraphCreateReader "createReader"
#define kShortcutDescActionGraphCreateReader "Create Reader"

#define kShortcutIDActionGraphCreateWriter "createWriter"
#define kShortcutDescActionGraphCreateWriter "Create Writer"

#define kShortcutIDActionGraphRearrangeNodes "rearrange"
#define kShortcutDescActionGraphRearrangeNodes "Rearrange Nodes"

#define kShortcutIDActionGraphRemoveNodes "remove"
#define kShortcutDescActionGraphRemoveNodes "Remove Nodes"

#define kShortcutIDActionGraphShowExpressions "displayExp"
#define kShortcutDescActionGraphShowExpressions "Show Expressions Links"

#define kShortcutIDActionGraphNavigateUpstream "navigateUp"
#define kShortcutDescActionGraphNavigateUpstream "Navigate Tree Upward"

#define kShortcutIDActionGraphNavigateDownstream "navigateDown"
#define kShortcutDescActionGraphNavigateDownstram "Navigate Tree Downward"

#define kShortcutIDActionGraphSelectUp "selUp"
#define kShortcutDescActionGraphSelectUp "Select Tree Upward"

#define kShortcutIDActionGraphSelectDown "selDown"
#define kShortcutDescActionGraphSelectDown "Select Tree Downward"

#define kShortcutIDActionGraphSelectAll "selectAll"
#define kShortcutDescActionGraphSelectAll "Select All Nodes"

#define kShortcutIDActionGraphSelectAllVisible "selectAllVisible"
#define kShortcutDescActionGraphSelectAllVisible "Select All Visible Nodes"

#define kShortcutIDActionGraphEnableHints "hints"
#define kShortcutDescActionGraphEnableHints "Enable Connection Hints"

#define kShortcutIDActionGraphAutoHideInputs "autoHideInputs"
#define kShortcutDescActionGraphAutoHideInputs "Auto-Hide Optional Inputs"

#define kShortcutIDActionGraphHideInputs "hideInputs"
#define kShortcutDescActionGraphHideInputs "Hide inputs"

#define kShortcutIDActionGraphSwitchInputs "switchInputs"
#define kShortcutDescActionGraphSwitchInputs "Switch Inputs 1 and 2"

#define kShortcutIDActionGraphCopy "copy"
#define kShortcutDescActionGraphCopy "Copy Nodes"

#define kShortcutIDActionGraphPaste "paste"
#define kShortcutDescActionGraphPaste "Paste Nodes"

#define kShortcutIDActionGraphClone "clone"
#define kShortcutDescActionGraphClone "Clone Nodes"

#define kShortcutIDActionGraphDeclone "declone"
#define kShortcutDescActionGraphDeclone "De-clone Nodes"

#define kShortcutIDActionGraphCut "cut"
#define kShortcutDescActionGraphCut "Cut Nodes"

#define kShortcutIDActionGraphDuplicate "duplicate"
#define kShortcutDescActionGraphDuplicate "Duplicate Nodes"

#define kShortcutIDActionGraphDisableNodes "disable"
#define kShortcutDescActionGraphDisableNodes "Disable Nodes"

#define kShortcutIDActionGraphToggleAutoPreview "toggleAutoPreview"
#define kShortcutDescActionGraphToggleAutoPreview "Toggle Auto Previews"

#define kShortcutIDActionGraphToggleAutoTurbo "toggleAutoTurbo"
#define kShortcutDescActionGraphToggleAutoTurbo "Toggle Auto Turbo"

#define kShortcutIDActionGraphTogglePreview "togglePreview"
#define kShortcutDescActionGraphTogglePreview "Toggle Preview Images"

#define kShortcutIDActionGraphForcePreview "preview"
#define kShortcutDescActionGraphForcePreview "Refresh Preview Images"

#define kShortcutIDActionGraphShowCacheSize "cacheSize"
#define kShortcutDescActionGraphShowCacheSize "Diplay Cache Memory Consumption"


#define kShortcutIDActionGraphFrameNodes "frameNodes"
#define kShortcutDescActionGraphFrameNodes "Center on All Nodes"

#define kShortcutIDActionGraphFindNode "findNode"
#define kShortcutDescActionGraphFindNode "Find"

#define kShortcutIDActionGraphRenameNode "renameNode"
#define kShortcutDescActionGraphRenameNode "Rename Node"

#define kShortcutIDActionGraphExtractNode "extractNode"
#define kShortcutDescActionGraphExtractNode "Extract Node"

#define kShortcutIDActionGraphMakeGroup "makeGroup"
#define kShortcutDescActionGraphMakeGroup "Group From Selection"

#define kShortcutIDActionGraphExpandGroup "expandGroup"
#define kShortcutDescActionGraphExpandGroup "Expand group"

///////////CURVEEDITOR SHORTCUTS
#define kShortcutIDActionCurveEditorRemoveKeys "remove"
#define kShortcutDescActionCurveEditorRemoveKeys "Delete Keyframes"

#define kShortcutIDActionCurveEditorConstant "constant"
#define kShortcutDescActionCurveEditorConstant "Constant Interpolation"

#define kShortcutIDActionCurveEditorLinear "linear"
#define kShortcutDescActionCurveEditorLinear "Linear Interpolation"

#define kShortcutIDActionCurveEditorSmooth "smooth"
#define kShortcutDescActionCurveEditorSmooth "Smooth Interpolation"

#define kShortcutIDActionCurveEditorCatmullrom "catmullrom"
#define kShortcutDescActionCurveEditorCatmullrom "Catmull-Rom Interpolation"

#define kShortcutIDActionCurveEditorCubic "cubic"
#define kShortcutDescActionCurveEditorCubic "Cubic Interpolation"

#define kShortcutIDActionCurveEditorHorizontal "horiz"
#define kShortcutDescActionCurveEditorHorizontal "Horizontal Interpolation"

#define kShortcutIDActionCurveEditorBreak "break"
#define kShortcutDescActionCurveEditorBreak "Break"

#define kShortcutIDActionCurveEditorSelectAll "selectAll"
#define kShortcutDescActionCurveEditorSelectAll "Select All Keyframes"

#define kShortcutIDActionCurveEditorCenter "center"
#define kShortcutDescActionCurveEditorCenter "Center on Curve"

#define kShortcutIDActionCurveEditorCopy "copy"
#define kShortcutDescActionCurveEditorCopy "Copy Keyframes"

#define kShortcutIDActionCurveEditorPaste "paste"
#define kShortcutDescActionCurveEditorPaste "Paste Keyframes"

// Dope Sheet Editor shortcuts
#define kShortcutIDActionDopeSheetEditorDeleteKeys "deleteKeys"
#define kShortcutDescActionDopeSheetEditorDeleteKeys "Delete selected keyframes"

#define kShortcutIDActionDopeSheetEditorFrameSelection "frameonselection"
#define kShortcutDescActionDopeSheetEditorFrameSelection "Frame on selection"

#define kShortcutIDActionDopeSheetEditorSelectAllKeyframes "selectall"
#define kShortcutDescActionDopeSheetEditorSelectAllKeyframes "Select all"

#define kShortcutIDActionDopeSheetEditorRenameNode "renamenode"
#define kShortcutDescActionDopeSheetEditorRenameNode "Rename node label"

#define kShortcutIDActionDopeSheetEditorCopySelectedKeyframes "copyselectedkeyframes"
#define kShortcutDescActionDopeSheetEditorCopySelectedKeyframes "Copy selected keyframes"

#define kShortcutIDActionDopeSheetEditorPasteKeyframes "pastekeyframes"
#define kShortcutDescActionDopeSheetEditorPasteKeyframes "Paste keyframes"

// Script editor shortcuts
#define kShortcutIDActionScriptEditorPrevScript "prevScript"
#define kShortcutDescActionScriptEditorPrevScript "Previous Script"

#define kShortcutIDActionScriptEditorNextScript "nextScript"
#define kShortcutDescActionScriptEditorNextScript "Next Script"

#define kShortcutIDActionScriptEditorClearHistory "clearHistory"
#define kShortcutDescActionScriptEditorClearHistory "Clear History"

#define kShortcutIDActionScriptExecScript "execScript"
#define kShortcutDescActionScriptExecScript "Execute Script"

#define kShortcutIDActionScriptClearOutput "clearOutput"
#define kShortcutDescActionScriptClearOutput "Clear Output Window"

#define kShortcutIDActionScriptShowOutput "showHideOutput"
#define kShortcutDescActionScriptShowOutput "Show/Hide Output Window"

class QWidget;

inline
QKeySequence
makeKeySequence(const Qt::KeyboardModifiers & modifiers,
                Qt::Key key)
{
    int keys = 0;

    if ( modifiers.testFlag(Qt::ControlModifier) ) {
        keys |= Qt::CTRL;
    }
    if ( modifiers.testFlag(Qt::ShiftModifier) ) {
        keys |= Qt::SHIFT;
    }
    if ( modifiers.testFlag(Qt::AltModifier) ) {
        keys |= Qt::ALT;
    }
    if ( modifiers.testFlag(Qt::MetaModifier) ) {
        keys |= Qt::META;
    }
    if (key != (Qt::Key)0) {
        keys |= key;
    }

    return QKeySequence(keys);
}

///This is tricky to do, what we do is we try to find the native strings of the modifiers
///in the sequence native's string. If we find them, we remove them. The last character
///is then the key symbol, we just have to call seq[0] to retrieve it.
inline void
extractKeySequence(const QKeySequence & seq,
                   Qt::KeyboardModifiers & modifiers,
                   Qt::Key & symbol)
{
    const QString nativeMETAStr = QKeySequence(Qt::META).toString(QKeySequence::NativeText);
    const QString nativeCTRLStr = QKeySequence(Qt::CTRL).toString(QKeySequence::NativeText);
    const QString nativeSHIFTStr = QKeySequence(Qt::SHIFT).toString(QKeySequence::NativeText);
    const QString nativeALTStr = QKeySequence(Qt::ALT).toString(QKeySequence::NativeText);
    QString nativeSeqStr = seq.toString(QKeySequence::NativeText);

    if (nativeSeqStr.indexOf(nativeMETAStr) != -1) {
        modifiers |= Qt::MetaModifier;
        nativeSeqStr = nativeSeqStr.remove(nativeMETAStr);
    }
    if (nativeSeqStr.indexOf(nativeCTRLStr) != -1) {
        modifiers |= Qt::ControlModifier;
        nativeSeqStr = nativeSeqStr.remove(nativeCTRLStr);
    }
    if (nativeSeqStr.indexOf(nativeSHIFTStr) != -1) {
        modifiers |= Qt::ShiftModifier;
        nativeSeqStr = nativeSeqStr.remove(nativeSHIFTStr);
    }
    if (nativeSeqStr.indexOf(nativeALTStr) != -1) {
        modifiers |= Qt::AltModifier;
        nativeSeqStr = nativeSeqStr.remove(nativeALTStr);
    }

    ///The nativeSeqStr now contains only the symbol
    QKeySequence newSeq(nativeSeqStr,QKeySequence::NativeText);
    if (newSeq.count() > 0) {
        symbol = (Qt::Key)newSeq[0];
    } else {
        symbol = (Qt::Key)0;
    }
}

class BoundAction
{
public:

    bool editable;
    QString grouping; //< the grouping of the action, such as CurveEditor/
    QString actionID; //< the unique ID within the grouping
    QString description; //< the description that will be in the shortcut editor
    
    //There might be multiple combinations
    std::list<Qt::KeyboardModifiers> modifiers; //< the keyboard modifiers that must be held down during the action
    std::list<Qt::KeyboardModifiers> defaultModifiers; //< the default keyboard modifiers
    Qt::KeyboardModifiers ignoreMask; ///Mask of modifiers to ignore for this shortcut
    
    BoundAction()
    : editable(true)
    , ignoreMask(Qt::NoModifier)
    {
    }

    virtual ~BoundAction()
    {
    }
};


class ActionWithShortcut
: public QAction
{
        
    QString _group;
    
protected:
    
    std::vector<std::pair<QString,QKeySequence> > _shortcuts;
    
public:
    
    ActionWithShortcut(const QString & group,
                       const QString & actionID,
                       const QString & actionDescription,
                       QObject* parent,
                       bool setShortcutOnAction = true);
    
    ActionWithShortcut(const QString & group,
                       const QStringList & actionIDs,
                       const QString & actionDescription,
                       QObject* parent,
                       bool setShortcutOnAction = true);

    
    virtual ~ActionWithShortcut();
    
    virtual void setShortcutWrapper(const QString& actionID, const QKeySequence& shortcut);

    
};

/**
 * @brief Set the widget's tooltip and append in the tooltip the shortcut associated to the action.
 * This will be dynamically changed when the user edits the shortcuts from the editor.
 **/
#define setTooltipWithShortcut(group,actionID,tooltip,widget) ( widget->addAction(new TooltipActionShortcut(group,actionID,tooltip,widget)) )
#define setTooltipWithShortcut2(group,actionIDs,tooltip,widget) ( widget->addAction(new TooltipActionShortcut(group,actionIDs,tooltip,widget)) )

class TooltipActionShortcut
: public ActionWithShortcut
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

    QWidget* _widget;
    QString _originalTooltip;
    bool _tooltipSetInternally;
    
public:
    
    /**
     * @brief Set a dynamic shortcut in the tooltip. Reference it with %1 where you want to place the shortcut.
     **/
    TooltipActionShortcut(const QString & group,
                          const QString & actionID,
                          const QString & toolip,
                          QWidget* parent);
    
    /**
     * @brief Same as above except that the tooltip can contain multiple shortcuts.
     * In that case the tooltip should reference shortcuts by doing so %1, %2 etc... where
     * %1 references the first actionID, %2 the second ,etc...
     **/
    TooltipActionShortcut(const QString & group,
                          const QStringList & actionIDs,
                          const QString & toolip,
                          QWidget* parent);
    
    virtual ~TooltipActionShortcut() {
        
    }
    
    virtual void setShortcutWrapper(const QString& actionID, const QKeySequence& shortcut) OVERRIDE FINAL;
    
private:
    
    virtual bool eventFilter(QObject* watched, QEvent* event) OVERRIDE FINAL;
    
    
    void setTooltipFromOriginalTooltip();
    
};

class KeyBoundAction
    : public BoundAction
{
public:

    //There might be multiple shortcuts
    std::list<Qt::Key> currentShortcut; //< the actual shortcut for the keybind
    std::list<Qt::Key> defaultShortcut; //< the default shortcut proposed by the dev team
    
    
    std::list<ActionWithShortcut*> actions; //< list of actions using this shortcut

    KeyBoundAction()
        : BoundAction()
        , currentShortcut()
        , defaultShortcut()
    {
    }

    virtual ~KeyBoundAction()
    {
    }

    void updateActionsShortcut()
    {
        for (std::list<ActionWithShortcut*>::iterator it = actions.begin(); it != actions.end(); ++it) {
            if (!modifiers.empty()) {
                (*it)->setShortcutWrapper( actionID, makeKeySequence(modifiers.front(), currentShortcut.front()) );
            }
        }
    }
};

class MouseAction
    : public BoundAction
{
public:

    Qt::MouseButton button; //< the button that must be held down for the action. This cannot be edited!

    MouseAction()
        : BoundAction()
        , button(Qt::NoButton)
    {
    }

    virtual ~MouseAction()
    {
    }
};


///All the shortcuts of a group matched against their
///internal id to find and match the action in the event handlers
typedef std::map<QString,BoundAction*> GroupShortcuts;

///All groups shortcuts mapped against the name of the group
typedef std::map<QString,GroupShortcuts> AppShortcuts;


#endif // ACTIONSHORTCUTS_H
