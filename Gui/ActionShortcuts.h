/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
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

#ifndef ACTIONSHORTCUTS_H
#define ACTIONSHORTCUTS_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

/**
 * @brief In this file all Natron's actions that can have their shortcut edited should be listed.
 **/

#include <map>
#include <list>
#include <vector>

#include "Global/Macros.h"

CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QKeyEvent>
#include <QMouseEvent>
#include <QtCore/QString>
#include <QAction>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Engine/PluginActionShortcut.h"
#include "Gui/GuiFwd.h"

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

#define kShortcutIDActionReloadProject "reloadProject"
#define kShortcutDescActionReloadProject "Reload Project"

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

#define kShortcutIDActionShowErrorLog "showErrorLog"
#define kShortcutDescActionShowErrorLog "Show Project Errors Log..."

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
#define kShortcutDescActionShowAbout "About Natron"

#define kShortcutIDActionRenderSelected "renderSelect"
#define kShortcutDescActionRenderSelected "Render Selected Writers"

#define kShortcutIDActionEnableRenderStats "enableRenderStats"
#define kShortcutDescActionEnableRenderStats "Enable Render Statistics"

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

#define kShortcutIDActionConnectViewerBToInput1 "connectViewerBInput1"
#define kShortcutDescActionConnectViewerBToInput1 "Connect Viewer B Side to Input 1"

#define kShortcutIDActionConnectViewerBToInput2 "connectViewerBInput2"
#define kShortcutDescActionConnectViewerBToInput2 "Connect Viewer B Side to Input 2"

#define kShortcutIDActionConnectViewerBToInput3 "connectViewerBInput3"
#define kShortcutDescActionConnectViewerBToInput3 "Connect Viewer B Side to Input 3"

#define kShortcutIDActionConnectViewerBToInput4 "connectViewerBInput4"
#define kShortcutDescActionConnectViewerBToInput4 "Connect Viewer B Side to Input 4"

#define kShortcutIDActionConnectViewerBToInput5 "connectViewerBInput5"
#define kShortcutDescActionConnectViewerBToInput5 "Connect Viewer B Side to Input 5"

#define kShortcutIDActionConnectViewerBToInput6 "connectViewerBInput6"
#define kShortcutDescActionConnectViewerBToInput6 "Connect Viewer B Side to Input 6"

#define kShortcutIDActionConnectViewerBToInput7 "connectViewerBInput7"
#define kShortcutDescActionConnectViewerBToInput7 "Connect Viewer B Side to Input 7"

#define kShortcutIDActionConnectViewerBToInput8 "connectViewerBInput8"
#define kShortcutDescActionConnectViewerBToInput8 "Connect Viewer B Side to Input 8"

#define kShortcutIDActionConnectViewerBToInput9 "connectViewerBInput9"
#define kShortcutDescActionConnectViewerBToInput9 "Connect Viewer B Side to Input 9"

#define kShortcutIDActionConnectViewerBToInput10 "connectViewerBInput10"
#define kShortcutDescActionConnectViewerBToInput10 "Connect Viewer B Side to Input 10"

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
#define kShortcutDescActionLuminanceA "Display Luminance on Input A Only"

#define kShortcutIDActionMatteOverlay "matteOverlay"
#define kShortcutDescActionMatteOverlay "Overlay Alpha Channel"

#define kShortcutIDActionRedA "channelRA"
#define kShortcutDescActionRedA "Display Red Channel on input A only"

#define kShortcutIDActionGreenA "channelGA"
#define kShortcutDescActionGreenA "Display Green Channel on Input A Only"

#define kShortcutIDActionBlueA "channelBA"
#define kShortcutDescActionBlueA "Display Blue Channel on Input A Only"

#define kShortcutIDActionAlphaA "channelAA"
#define kShortcutDescActionAlphaA "Display Alpha Channel on Input A Only"

#define kShortcutIDActionFitViewer "fitViewer"
#define kShortcutDescActionFitViewer "Fit Image to Viewer"

#define kShortcutIDActionClipEnabled "clipEnabled"
#define kShortcutDescActionClipEnabled "Enable Clipping to Project Window"

#define kShortcutIDActionFullFrameProc "fullFrameProc"
#define kShortcutDescActionFullFrameProc "Full Frame Processing"

#define kShortcutIDActionRefresh "refresh"
#define kShortcutDescActionRefresh "Refresh Image"

#define kShortcutIDActionRefreshWithStats "refreshWithStats"
#define kShortcutDescActionRefreshWithStats "Refresh Image and Show Render Statistics"

#define kShortcutIDActionROIEnabled "userRoiEnabled"
#define kShortcutDescActionROIEnabled "Enable User RoI"

#define kShortcutIDActionNewROI "newRoi"
#define kShortcutDescActionNewROI "New User RoI"

#define kShortcutIDActionPauseViewer "pauseUpdates"
#define kShortcutDescActionPauseViewer "Pause Updates"

#define kShortcutIDActionPauseViewerInputA "pauseUpdatesA"
#define kShortcutDescActionPauseViewerInputA "Pause Updates on Input A Only"

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
#define kShortcutDescActionHideInfobar "Show/Hide Info Bar"

#define kShortcutIDActionHideAll "hideAll"
#define kShortcutDescActionHideAll "Hide All"

#define kShortcutIDActionShowAll "showAll"
#define kShortcutDescActionShowAll "Show All"

#define kShortcutIDMousePickColor "pick"
#define kShortcutDescMousePickColor "Pick a Color"

#define kShortcutIDMousePickInputColor "pickInput"
#define kShortcutDescMousePickInputColor "Pick a Color from input of viewed node"

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

#define kShortcutIDPrevView "prevView"
#define kShortcutDescPrevView "Previous View"

#define kShortcutIDNextView "nextView"
#define kShortcutDescNextView "Next View"
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

#define kShortcutIDActionPlayerPlaybackIn "pbIn"
#define kShortcutDescActionPlayerPlaybackIn "Set Playback \"In\" Point"

#define kShortcutIDActionPlayerPlaybackOut "pbOut"
#define kShortcutDescActionPlayerPlaybackOut "Set Playback \"Out\" Point"


///////////NODEGRAPH SHORTCUTS
#ifndef NATRON_ENABLE_IO_META_NODES

#define kShortcutIDActionGraphCreateReader "createReader"
#define kShortcutDescActionGraphCreateReader "Create Reader"

#define kShortcutIDActionGraphCreateWriter "createWriter"
#define kShortcutDescActionGraphCreateWriter "Create Writer"

#endif // #ifdef NATRON_ENABLE_IO_META_NODES

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

#define kShortcutIDActionGraphOpenNodePanel "openSettingsPanel"
#define kShortcutDescActionGraphOpenNodePanel "Open Node Settings Panel"

#define kShortcutIDActionGraphFrameNodes "frameNodes"
#define kShortcutDescActionGraphFrameNodes "Center on All Nodes"

#define kShortcutIDActionGraphFindNode "findNode"
#define kShortcutDescActionGraphFindNode "Find"

#define kShortcutIDActionGraphRenameNode "renameNode"
#define kShortcutDescActionGraphRenameNode "Rename Node"

#define kShortcutIDActionGraphExtractNode "extractNode"
#define kShortcutDescActionGraphExtractNode "Extract Node"

#define kShortcutIDActionGraphMakeGroup "makeGroup"
#define kShortcutDescActionGraphMakeGroup "Group from Selection"

#define kShortcutIDActionGraphExpandGroup "expandGroup"
#define kShortcutDescActionGraphExpandGroup "Expand Group"

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

#define kShortcutIDActionCurveEditorCenterAll "frameAll"
#define kShortcutDescActionCurveEditorCenterAll "Frame All Curves"

#define kShortcutIDActionCurveEditorCenter "center"
#define kShortcutDescActionCurveEditorCenter "Center on Curve"

#define kShortcutIDActionCurveEditorCopy "copy"
#define kShortcutDescActionCurveEditorCopy "Copy Keyframes"

#define kShortcutIDActionCurveEditorPaste "paste"
#define kShortcutDescActionCurveEditorPaste "Paste Keyframes"

// Dope Sheet Editor shortcuts
#define kShortcutIDActionDopeSheetEditorDeleteKeys "deleteKeys"
#define kShortcutDescActionDopeSheetEditorDeleteKeys "Delete Selected Keyframes"

#define kShortcutIDActionDopeSheetEditorFrameSelection "frameonselection"
#define kShortcutDescActionDopeSheetEditorFrameSelection "Frame on Selection"

#define kShortcutIDActionDopeSheetEditorSelectAllKeyframes "selectall"
#define kShortcutDescActionDopeSheetEditorSelectAllKeyframes "Select All"

#define kShortcutIDActionDopeSheetEditorRenameNode "renamenode"
#define kShortcutDescActionDopeSheetEditorRenameNode "Rename Node"

#define kShortcutIDActionDopeSheetEditorCopySelectedKeyframes "copyselectedkeyframes"
#define kShortcutDescActionDopeSheetEditorCopySelectedKeyframes "Copy Selected Keyframes"

#define kShortcutIDActionDopeSheetEditorPasteKeyframes "pastekeyframes"
#define kShortcutDescActionDopeSheetEditorPasteKeyframes "Paste Keyframes"

#define kShortcutIDActionDopeSheetEditorPasteKeyframesAbsolute "pastekeyframesAbs"
#define kShortcutDescActionDopeSheetEditorPasteKeyframesAbsolute "Paste Keyframes Absolute"

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

NATRON_NAMESPACE_ENTER

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
    if ( modifiers.testFlag(Qt::KeypadModifier) ) {
        keys |= Qt::KeypadModifier;
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
    const QString nativeKeypadStr = QKeySequence(Qt::KeypadModifier).toString(QKeySequence::NativeText);
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
    if (!nativeKeypadStr.isEmpty() && nativeSeqStr.indexOf(nativeKeypadStr) != -1) {
        modifiers |= Qt::KeypadModifier;
        nativeSeqStr = nativeSeqStr.remove(nativeKeypadStr);
    }

    ///The nativeSeqStr now contains only the symbol
    QKeySequence newSeq(nativeSeqStr, QKeySequence::NativeText);
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
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

private:
    QString _group;

protected:

    std::vector<std::pair<QString, QKeySequence> > _shortcuts;

public:

    ActionWithShortcut(const std::string & group,
                       const std::string & actionID,
                       const std::string & actionDescription,
                       QObject* parent,
                       bool setShortcutOnAction = true);


    ActionWithShortcut(const std::string & group,
                       const std::list<std::string> & actionIDs,
                       const std::string & actionDescription,
                       QObject* parent,
                       bool setShortcutOnAction = true);


    const std::vector<std::pair<QString, QKeySequence> >& getShortcuts() const
    {
        return _shortcuts;
    }

    virtual ~ActionWithShortcut();
    virtual void setShortcutWrapper(const QString& actionID, const QKeySequence& shortcut);
};

/**
 * @brief Set the widget's tooltip and append in the tooltip the shortcut associated to the action.
 * This will be dynamically changed when the user edits the shortcuts from the editor.
 **/
#define setToolTipWithShortcut(group, actionID, tooltip, widget) ( widget->addAction( new ToolTipActionShortcut(group, actionID, tooltip, widget) ) )
#define setToolTipWithShortcut2(group, actionIDs, tooltip, widget) ( widget->addAction( new ToolTipActionShortcut(group, actionIDs, tooltip, widget) ) )

class ToolTipActionShortcut
    : public ActionWithShortcut
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

    QWidget* _widget;
    QString _originalToolTip;
    bool _tooltipSetInternally;

public:

    /**
     * @brief Set a dynamic shortcut in the tooltip. Reference it with %1 where you want to place the shortcut.
     **/
    ToolTipActionShortcut(const std::string & group,
                          const std::string & actionID,
                          const std::string & toolip,
                          QWidget* parent);

    /**
     * @brief Same as above except that the tooltip can contain multiple shortcuts.
     * In that case the tooltip should reference shortcuts by doing so %1, %2 etc... where
     * %1 references the first actionID, %2 the second ,etc...
     **/
    ToolTipActionShortcut(const std::string & group,
                          const std::list<std::string> & actionIDs,
                          const std::string & toolip,
                          QWidget* parent);

    virtual ~ToolTipActionShortcut()
    {
    }

    virtual void setShortcutWrapper(const QString& actionID, const QKeySequence& shortcut) OVERRIDE FINAL;

private:

    virtual bool eventFilter(QObject* watched, QEvent* event) OVERRIDE FINAL;


    void setToolTipFromOriginalToolTip();
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
            if ( !modifiers.empty() ) {
                (*it)->setShortcutWrapper( actionID, makeKeySequence( modifiers.front(), currentShortcut.front() ) );
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
typedef std::map<QString, BoundAction*> GroupShortcuts;

///All groups shortcuts mapped against the name of the group
typedef std::map<QString, GroupShortcuts> AppShortcuts;

NATRON_NAMESPACE_EXIT

#endif // ACTIONSHORTCUTS_H
