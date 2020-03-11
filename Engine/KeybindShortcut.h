/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * Copyright (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
 * Copyright (C) 2018-2020 The Natron developers
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

#ifndef NATRON_ENGINE_KEYBINDSHORTCUT_H
#define NATRON_ENGINE_KEYBINDSHORTCUT_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include <list>
#include <string>

#include "Global/KeySymbols.h"

#include "Engine/EngineFwd.h"


#define kShortcutGroupGlobal "Global"
#define kShortcutGroupNodegraph "Node Graph"
#define kShortcutGroupAnimationModule "Animation Editor"
#define kShortcutGroupPlayer "Player"
#define kShortcutGroupNodes "Nodes"
#define kShortcutGroupScriptEditor "Script Editor"

/////////GLOBAL SHORTCUTS
#define kShortcutActionNewProject "newProject"
#define kShortcutActionNewProjectLabel "New Project"
#define kShortcutActionNewProjectHint "Create a new project"

#define kShortcutActionOpenProject "openProject"
#define kShortcutActionOpenProjectLabel "Open Project..."
#define kShortcutActionOpenProjectHint "Open an existing project"

#define kShortcutActionCloseProject "closeProject"
#define kShortcutActionCloseProjectLabel "Close Project"
#define kShortcutActionCloseProjectHint "Close the current project"

#define kShortcutActionReloadProject "reloadProject"
#define kShortcutActionReloadProjectLabel "Reload Project"
#define kShortcutActionReloadProjectHint "Reload the current project from the file"

#define kShortcutActionSaveProject "saveProject"
#define kShortcutActionSaveProjectLabel "Save Project"
#define kShortcutActionSaveProjectHint "Save the current project"

#define kShortcutActionSaveAsProject "saveAsProject"
#define kShortcutActionSaveAsProjectLabel "Save Project As.."
#define kShortcutActionSaveAsProjectHint "Save the project to a new file"

#define kShortcutActionSaveAndIncrVersion "saveAndIncr"
#define kShortcutActionSaveAndIncrVersionLabel "New Project Version"
#define kShortcutActionSaveAndIncrVersionHint "Same as Save As, except that the new file will be the name of the current file with a version increment"

#define kShortcutActionPreferences "preferences"
#define kShortcutActionPreferencesLabel "Preferences..."
#define kShortcutActionPreferencesHint "Show the application preferences window"

#define kShortcutActionQuit "quit"
#define kShortcutActionQuitLabel "Quit"
#define kShortcutActionQuitHint "Quit " NATRON_APPLICATION_NAME

#define kShortcutActionProjectSettings "projectSettings"
#define kShortcutActionProjectSettingsLabel "Show Project Settings..."
#define kShortcutActionProjectSettingsHint "Show the project settings panel"

#define kShortcutActionShowErrorLog "showErrorLog"
#define kShortcutActionShowErrorLogLabel "Show Project Errors Log..."
#define kShortcutActionShowErrorLogHint "Show the project errors log window"

#define kShortcutActionNewViewer "newViewer"
#define kShortcutActionNewViewerLabel "New Viewer"
#define kShortcutActionNewViewerHint "Create a new viewer"

#define kShortcutActionFullscreen "fullScreen"
#define kShortcutActionFullscreenLabel "Enter Full Screen"
#define kShortcutActionFullscreenHint "Show the window full-screen"

#define kShortcutActionShowWindowsConsole "showApplicationConsole"
#define kShortcutActionShowWindowsConsoleLabel "Show/Hide Application Console"
#define kShortcutActionShowWindowsConsoleHint "Show/Hide the background application console"

#define kShortcutActionClearCache "clearCache"
#define kShortcutActionClearCacheLabel "Clear Cache"
#define kShortcutActionClearCacheHint "Clear the application main cache"

#define kShortcutActionClearPluginsLoadCache "clearPluginsCache"
#define kShortcutActionClearPluginsLoadCacheLabel "Clear Plug-ins Load Cache"
#define kShortcutActionClearPluginsLoadCacheHint "Removes the plug-ins loaded cache. Subsequent launch of Natron will reload all plug-ins from the disk."

#define kShortcutActionShowCacheReport "showCacheReport"
#define kShortcutActionShowCacheReportLabel "Show Cache Report"
#define kShortcutActionShowCacheReportHint "Show a window with a report about the cache memory usage"

#define kShortcutActionShowAbout "showAbout"
#define kShortcutActionShowAboutLabel "About Natron"
#define kShortcutActionShowAboutHint "Show the about window"

#define kShortcutActionRenderSelected "renderSelect"
#define kShortcutActionRenderSelectedLabel "Render Selected Writer(s)"
#define kShortcutActionRenderSelectedHint "Render the selected writer node(s). If multiple writers are selected, they will either render simultaneously or one after the other depending on the queuing setting in the preferences"

#define kShortcutActionRenderAll "renderAll"
#define kShortcutActionRenderAllLabel "Render All Writers"
#define kShortcutActionRenderAllHint "Render all writer nodes in the project. If multiple writers are selected, they will either render simultaneously or one after the other depending on the queuing setting in the preferences"

#define kShortcutActionEnableRenderStats "enableRenderStats"
#define kShortcutActionEnableRenderStatsLabel "Enable Render Statistics"
#define kShortcutActionEnableRenderStatsHint "Show a statistics window, subsequent renders will display useful timing informations about nodes"

#define kShortcutActionConnectViewerToInput1 "connectViewerInput1"
#define kShortcutActionConnectViewerToInput1Label "Connect Viewer to Input 1"
#define kShortcutActionConnectViewerToInput1Hint "Connect Viewer to Input 1"

#define kShortcutActionConnectViewerToInput2 "connectViewerInput2"
#define kShortcutActionConnectViewerToInput2Label "Connect Viewer to Input 2"
#define kShortcutActionConnectViewerToInput2Hint "Connect Viewer to Input 2"

#define kShortcutActionConnectViewerToInput3 "connectViewerInput3"
#define kShortcutActionConnectViewerToInput3Label "Connect Viewer to Input 3"
#define kShortcutActionConnectViewerToInput3Hint "Connect Viewer to Input 3"

#define kShortcutActionConnectViewerToInput4 "connectViewerInput4"
#define kShortcutActionConnectViewerToInput4Label "Connect Viewer to Input 4"
#define kShortcutActionConnectViewerToInput4Hint "Connect Viewer to Input 4"

#define kShortcutActionConnectViewerToInput5 "connectViewerInput5"
#define kShortcutActionConnectViewerToInput5Label "Connect Viewer to Input 5"
#define kShortcutActionConnectViewerToInput5Hint "Connect Viewer to Input 5"

#define kShortcutActionConnectViewerToInput6 "connectViewerInput6"
#define kShortcutActionConnectViewerToInput6Label "Connect Viewer to Input 6"
#define kShortcutActionConnectViewerToInput6Hint "Connect Viewer to Input 6"

#define kShortcutActionConnectViewerToInput7 "connectViewerInput7"
#define kShortcutActionConnectViewerToInput7Label "Connect Viewer to Input 7"
#define kShortcutActionConnectViewerToInput7Hint "Connect Viewer to Input 7"

#define kShortcutActionConnectViewerToInput8 "connectViewerInput8"
#define kShortcutActionConnectViewerToInput8Label "Connect Viewer to Input 8"
#define kShortcutActionConnectViewerToInput8Hint "Connect Viewer to Input 8"

#define kShortcutActionConnectViewerToInput9 "connectViewerInput9"
#define kShortcutActionConnectViewerToInput9Label "Connect Viewer to Input 9"
#define kShortcutActionConnectViewerToInput9Hint "Connect Viewer to Input 9"

#define kShortcutActionConnectViewerToInput10 "connectViewerInput10"
#define kShortcutActionConnectViewerToInput10Label "Connect Viewer to Input 10"
#define kShortcutActionConnectViewerToInput10Hint "Connect Viewer to Input 10"

#define kShortcutActionConnectViewerBToInput1 "connectViewerBInput1"
#define kShortcutActionConnectViewerBToInput1Label "Connect Viewer B Side to Input 1"
#define kShortcutActionConnectViewerBToInput1Hint "Connect Viewer B Side to Input 1"

#define kShortcutActionConnectViewerBToInput2 "connectViewerBInput2"
#define kShortcutActionConnectViewerBToInput2Label "Connect Viewer B Side to Input 2"
#define kShortcutActionConnectViewerBToInput2Hint "Connect Viewer B Side to Input 2"

#define kShortcutActionConnectViewerBToInput3 "connectViewerBInput3"
#define kShortcutActionConnectViewerBToInput3Label "Connect Viewer B Side to Input 3"
#define kShortcutActionConnectViewerBToInput3Hint "Connect Viewer B Side to Input 3"

#define kShortcutActionConnectViewerBToInput4 "connectViewerBInput4"
#define kShortcutActionConnectViewerBToInput4Label "Connect Viewer B Side to Input 4"
#define kShortcutActionConnectViewerBToInput4Hint "Connect Viewer B Side to Input 4"

#define kShortcutActionConnectViewerBToInput5 "connectViewerBInput5"
#define kShortcutActionConnectViewerBToInput5Label "Connect Viewer B Side to Input 5"
#define kShortcutActionConnectViewerBToInput5Hint "Connect Viewer B Side to Input 5"

#define kShortcutActionConnectViewerBToInput6 "connectViewerBInput6"
#define kShortcutActionConnectViewerBToInput6Label "Connect Viewer B Side to Input 6"
#define kShortcutActionConnectViewerBToInput6Hint "Connect Viewer B Side to Input 6"

#define kShortcutActionConnectViewerBToInput7 "connectViewerBInput7"
#define kShortcutActionConnectViewerBToInput7Label "Connect Viewer B Side to Input 7"
#define kShortcutActionConnectViewerBToInput7Hint "Connect Viewer B Side to Input 7"

#define kShortcutActionConnectViewerBToInput8 "connectViewerBInput8"
#define kShortcutActionConnectViewerBToInput8Label "Connect Viewer B Side to Input 8"
#define kShortcutActionConnectViewerBToInput8Hint "Connect Viewer B Side to Input 8"

#define kShortcutActionConnectViewerBToInput9 "connectViewerBInput9"
#define kShortcutActionConnectViewerBToInput9Label "Connect Viewer B Side to Input 9"
#define kShortcutActionConnectViewerBToInput9Hint "Connect Viewer B Side to Input 9"

#define kShortcutActionConnectViewerBToInput10 "connectViewerBInput10"
#define kShortcutActionConnectViewerBToInput10Label "Connect Viewer B Side to Input 10"
#define kShortcutActionConnectViewerBToInput10Hint "Connect Viewer B Side to Input 10"

#define kShortcutActionShowPaneFullScreen "showPaneFullScreen"
#define kShortcutActionShowPaneFullScreenLabel "Show Pane Full Screen"
#define kShortcutActionShowPaneFullScreenHint "Temporarily show a panel full-screen"

#define kShortcutActionImportLayout "importLayout"
#define kShortcutActionImportLayoutLabel "Import Layout..."
#define kShortcutActionImportLayoutHint "Import a Natron workspace file"

#define kShortcutActionExportLayout "exportLayout"
#define kShortcutActionExportLayoutLabel "Export Layout..."
#define kShortcutActionExportLayoutHint "Export the current workspace layout to a file"

#define kShortcutActionDefaultLayout "restoreDefaultLayout"
#define kShortcutActionDefaultLayoutLabel "Restore Default Layout"
#define kShortcutActionDefaultLayoutHint "Reset the workspace to the default layout. If a default layout file is set in the preferences, the layout of this file will be restored."

#define kShortcutActionNextTab "nextTab"
#define kShortcutActionNextTabLabel "Next Tab"
#define kShortcutActionNextTabHint "Make the next tab in the panel the current tab"

#define kShortcutActionPrevTab "prevTab"
#define kShortcutActionPrevTabLabel "Previous Tab"
#define kShortcutActionPrevTabHint "Make the previous tab in the panel the current tab"

#define kShortcutActionCloseTab "closeTab"
#define kShortcutActionCloseTabLabel "Close Tab"
#define kShortcutActionCloseTabHint "Close the current tab, removing it from the panel"

///////////NODEGRAPH SHORTCUTS

#define kShortcutActionGraphRearrangeNodes "rearrange"
#define kShortcutActionGraphRearrangeNodesLabel "Rearrange Nodes"
#define kShortcutActionGraphRearrangeNodesHint "Organize nodes so that they are displayed more linearly"

#define kShortcutActionGraphRemoveNodes "remove"
#define kShortcutActionGraphRemoveNodesLabel "Remove Nodes"
#define kShortcutActionGraphRemoveNodesHint "Remove the selected node(s)"

#define kShortcutActionGraphShowExpressions "displayExp"
#define kShortcutActionGraphShowExpressionsLabel "Show Links"
#define kShortcutActionGraphShowExpressionsHint "When checked, the green links between nodes indicating that some parameter(s) have an expression/link are visible"

#define kShortcutActionGraphSelectUp "selUp"
#define kShortcutActionGraphSelectUpLabel "Select Tree Upward"
#define kShortcutActionGraphSelectUpHint "Select the input of the currently selected node"

#define kShortcutActionGraphSelectDown "selDown"
#define kShortcutActionGraphSelectDownLabel "Select Tree Downward"
#define kShortcutActionGraphSelectDownHint "Select the output of the currently selected node"

#define kShortcutActionGraphSelectAll "selectAll"
#define kShortcutActionGraphSelectAllLabel "Select All Nodes"
#define kShortcutActionGraphSelectAllHint "Select all nodes in the nodegraph"

#define kShortcutActionGraphSelectAllVisible "selectAllVisible"
#define kShortcutActionGraphSelectAllVisibleLabel "Select All Visible Nodes"
#define kShortcutActionGraphSelectAllVisibleHint "Select all visible nodes in the nodegraph"

#define kShortcutActionGraphAutoHideInputs "autoHideInputs"
#define kShortcutActionGraphAutoHideInputsLabel "Auto-Hide Optional Inputs"
#define kShortcutActionGraphAutoHideInputsHint "When checked, optional node inputs (such as masks) will be hidden when the node is not selected"

#define kShortcutActionGraphHideInputs "hideInputs"
#define kShortcutActionGraphHideInputsLabel "Hide inputs"
#define kShortcutActionGraphHideInputsHint "When checked, all node inputs will be invisible in the nodegraph"

#define kShortcutActionGraphSwitchInputs "switchInputs"
#define kShortcutActionGraphSwitchInputsLabel "Switch Inputs 1 and 2"
#define kShortcutActionGraphSwitchInputsHint "If the selected node has 2 inputs, switch their input nodes"

#define kShortcutActionGraphCopy "copy"
#define kShortcutActionGraphCopyLabel "Copy Nodes"
#define kShortcutActionGraphCopyHint "Copy the selected node(s) to the clipboard"

#define kShortcutActionGraphPaste "paste"
#define kShortcutActionGraphPasteLabel "Paste Nodes"
#define kShortcutActionGraphPasteHint "Paste the selected node(s) from the clipboard"

#define kShortcutActionGraphClone "clone"
#define kShortcutActionGraphCloneLabel "Clone Nodes"
#define kShortcutActionGraphCloneHint "Clone the selected node(s)"

#define kShortcutActionGraphDeclone "declone"
#define kShortcutActionGraphDecloneLabel "Unclone"
#define kShortcutActionGraphDecloneHint "Remove any clone link from the selected node(s)"

#define kShortcutActionGraphCut "cut"
#define kShortcutActionGraphCutLabel "Cut Nodes"
#define kShortcutActionGraphCutHint "Remove the selected node(s) and copy them to the clipboard"

#define kShortcutActionGraphDuplicate "duplicate"
#define kShortcutActionGraphDuplicateLabel "Duplicate Nodes"
#define kShortcutActionGraphDuplicateHint "Duplicate the selected node(s) in the nodegraph"

#define kShortcutActionGraphDisableNodes "disable"
#define kShortcutActionGraphDisableNodesLabel "Disable Nodes"
#define kShortcutActionGraphDisableNodesHint "Disable the selected node(s)"

#define kShortcutActionGraphToggleAutoPreview "toggleAutoPreview"
#define kShortcutActionGraphToggleAutoPreviewLabel "Toggle Auto Previews"
#define kShortcutActionGraphToggleAutoPreviewHint "Toggles auto preview refresh on nodes"

#define kShortcutActionGraphToggleAutoTurbo "toggleAutoTurbo"
#define kShortcutActionGraphToggleAutoTurboLabel "Toggle Auto Turbo"
#define kShortcutActionGraphToggleAutoTurboHint "Toggles auto turbo enabling. This will check the Auto Turbo button on the timeline automatically when during playback"

#define kShortcutActionGraphTogglePreview "togglePreview"
#define kShortcutActionGraphTogglePreviewLabel "Toggle Preview Images"
#define kShortcutActionGraphTogglePreviewHint "Show or hide the preview image on selected node(s)"

#define kShortcutActionGraphForcePreview "preview"
#define kShortcutActionGraphForcePreviewLabel "Refresh Preview Images"
#define kShortcutActionGraphForcePreviewHint "Force a refresh of the preview image on the selected node(s)"

#define kShortcutActionGraphShowCacheSize "cacheSize"
#define kShortcutActionGraphShowCacheSizeLabel "Display Cache Memory Consumption"
#define kShortcutActionGraphShowCacheSizeHint "Display the memory currently taken by the cache on disk"

#define kShortcutActionGraphOpenNodePanel "openSettingsPanel"
#define kShortcutActionGraphOpenNodePanelLabel "Open Node Settings Panel"
#define kShortcutActionGraphOpenNodePanelHint "Opens the selected node settings panel"

#define kShortcutActionGraphFrameNodes "frameNodes"
#define kShortcutActionGraphFrameNodesLabel "Center on All Nodes"
#define kShortcutActionGraphFrameNodesHint "Center the nodegraph view on all nodes"

#define kShortcutActionGraphFindNode "findNode"
#define kShortcutActionGraphFindNodeLabel "Find"
#define kShortcutActionGraphFindNodeHint "Show the find window"

#define kShortcutActionGraphRenameNode "renameNode"
#define kShortcutActionGraphRenameNodeLabel "Rename Node"
#define kShortcutActionGraphRenameNodeHint "Show the rename window"

#define kShortcutActionGraphExtractNode "extractNode"
#define kShortcutActionGraphExtractNodeLabel "Extract Node"
#define kShortcutActionGraphExtractNodeHint "Extract the selected node from the graph and disconnect it from its inputs/outputs"

#define kShortcutActionGraphMakeGroup "makeGroup"
#define kShortcutActionGraphMakeGroupLabel "Group from Selection"
#define kShortcutActionGraphMakeGroupHint "Create a group from the currently selected nodes and move them to this group."

#define kShortcutActionGraphExpandGroup "expandGroup"
#define kShortcutActionGraphExpandGroupLabel "Expand Group"
#define kShortcutActionGraphExpandGroupHint "Expands the selected group node(s), replacing the Group node by its internal sub-graph"

///////////CURVEEDITOR SHORTCUTS
#define kShortcutActionAnimationModuleRemoveKeys "remove"
#define kShortcutActionAnimationModuleRemoveKeysLabel "Remove Keyframes"
#define kShortcutActionAnimationModuleRemoveKeysHint "Remove the selected keyframes"

#define kShortcutActionAnimationModuleConstant "constant"
#define kShortcutActionAnimationModuleConstantLabel "Constant Interpolation"
#define kShortcutActionAnimationModuleConstantHint "Set the selected keyframes to a constant interpolation"

#define kShortcutActionAnimationModuleLinear "linear"
#define kShortcutActionAnimationModuleLinearLabel "Linear Interpolation"
#define kShortcutActionAnimationModuleLinearHint "Set the selected keyframes to a linear interpolation"

#define kShortcutActionAnimationModuleSmooth "smooth"
#define kShortcutActionAnimationModuleSmoothLabel "Smooth Interpolation"
#define kShortcutActionAnimationModuleSmoothHint "Set the selected keyframes to a smooth interpolation"

#define kShortcutActionAnimationModuleCatmullrom "catmullrom"
#define kShortcutActionAnimationModuleCatmullromLabel "Catmull-Rom Interpolation"
#define kShortcutActionAnimationModuleCatmullromHint "Set the selected keyframes to a catmull-rom interpolation"

#define kShortcutActionAnimationModuleCubic "cubic"
#define kShortcutActionAnimationModuleCubicLabel "Cubic Interpolation"
#define kShortcutActionAnimationModuleCubicHint "Set the selected keyframes to a cubi interpolation"

#define kShortcutActionAnimationModuleHorizontal "horiz"
#define kShortcutActionAnimationModuleHorizontalLabel "Horizontal Interpolation"
#define kShortcutActionAnimationModuleHorizontalHint "Set the selected keyframes to a horizontal interpolation"

#define kShortcutActionAnimationModuleBreak "break"
#define kShortcutActionAnimationModuleBreakLabel "Break Tangents"
#define kShortcutActionAnimationModuleBreakHint "Set the selected keyframes so both tangents can be set freely"

#define kShortcutActionAnimationModuleSelectAll "selectAll"
#define kShortcutActionAnimationModuleSelectAllLabel "Select All"
#define kShortcutActionAnimationModuleSelectAllHint "Select all keyframes"

#define kShortcutActionAnimationModuleCenterAll "centerAll"
#define kShortcutActionAnimationModuleCenterAllLabel "Center on All Items"
#define kShortcutActionAnimationModuleCenterAllHint "Center the view on all keyframes"

#define kShortcutActionAnimationModuleCenter "center"
#define kShortcutActionAnimationModuleCenterLabel "Center on Selected Items"
#define kShortcutActionAnimationModuleCenterHint "Center the view on the selected keyframes"

#define kShortcutActionAnimationModuleCopy "copy"
#define kShortcutActionAnimationModuleCopyLabel "Copy Selected Keyframes"
#define kShortcutActionAnimationModuleCopyHint "Copy the selected keyframes to the clipboard"

#define kShortcutActionAnimationModulePasteKeyframes "pastekeyframes"
#define kShortcutActionAnimationModulePasteKeyframesLabel "Paste Keyframes"
#define kShortcutActionAnimationModulePasteKeyframesHint "Paste the selected keyframes from the clipboard, relative to the timeline's current time"

#define kShortcutActionAnimationModulePasteKeyframesAbsolute "pastekeyframesAbs"
#define kShortcutActionAnimationModulePasteKeyframesAbsoluteLabel "Paste Keyframes Absolute"
#define kShortcutActionAnimationModulePasteKeyframesAbsoluteHint "Paste the selected keyframes from the clipboard with their absolute original time"

#define kShortcutActionAnimationModuleStackView "stackAnimView"
#define kShortcutActionAnimationModuleStackViewLabel "Show Curve Editor + Dope Sheet"
#define kShortcutActionAnimationModuleStackViewHint "Show both the curve editor and the dope sheet"

#define kShortcutActionAnimationModuleShowOnlyAnimated "showOnlyAnimated"
#define kShortcutActionAnimationModuleShowOnlyAnimatedLabel "Show Only Animated"
#define kShortcutActionAnimationModuleShowOnlyAnimatedHint "When checked, only the animated parameters will appear in the animation module"

// Script editor shortcuts
#define kShortcutActionScriptEditorPrevScript "prevScript"
#define kShortcutActionScriptEditorPrevScriptLabel "Previous Script"
#define kShortcutActionScriptEditorPrevScriptHint "Navigate backwards in the history of executed script in the input console"

#define kShortcutActionScriptEditorNextScript "nextScript"
#define kShortcutActionScriptEditorNextScriptLabel "Next Script"
#define kShortcutActionScriptEditorNextScriptHint "Navigate forwards in the history of executed script in the input console"

#define kShortcutActionScriptEditorClearHistory "clearHistory"
#define kShortcutActionScriptEditorClearHistoryLabel "Clear History"
#define kShortcutActionScriptEditorClearHistoryHint "Clears the console commands history"

#define kShortcutActionScriptExecScript "execScript"
#define kShortcutActionScriptExecScriptLabel "Execute Script"
#define kShortcutActionScriptExecScriptHint "Executes a script in the console"

#define kShortcutActionScriptClearOutput "clearOutput"
#define kShortcutActionScriptClearOutputLabel "Clear Output Window"
#define kShortcutActionScriptClearOutputHint "Clears the output window "

#define kShortcutActionScriptShowOutput "showHideOutput"
#define kShortcutActionScriptShowOutputLabel "Show/Hide Output Window"
#define kShortcutActionScriptShowOutputHint "Show/Hide output window"


NATRON_NAMESPACE_ENTER

class KeybindListenerI
{
public:

    KeybindListenerI()
    {

    }

    virtual ~KeybindListenerI()
    {

    }

    virtual void onShortcutChanged(const std::string& actionID,
                                   Key symbol,
                                   const KeyboardModifiers& modifiers) = 0;

};

class KeybindShortcut
{
public:

     // The grouping of the action (NodeGraph, MainWindow, Viewer, Tracking ...)
    std::string grouping;

    // A unique identifier for the action (not visible in the gui) within the grouping
    std::string actionID;

    // A label for the action, visible in the shortcut editor
    std::string actionLabel;

    // A description of the action, visible in the shortcut editor
    std::string description;

    // The keyboard modifiers that must be held down during the action
    // This can be changed by the user
    KeyboardModifiers modifiers;

    // The default value for modifiers. This is never changed.
    KeyboardModifiers defaultModifiers;

    // Mask of modifiers to ignore for this shortcut (some keyboards might require that modifiers are held
    // to produce a symbol)
    KeyboardModifiers ignoreMask;

    // The actual shortcut for the keybind.
    // This can be modified by the user
    Key currentShortcut;

    // The default value for the shortcut. This cannot be changed.
    Key defaultShortcut;

    // List of actions using this shortcut: they will be updated when this action changes.
    std::list<KeybindListenerI*> actions;

    // True if this action can be edited from the shortcut editor.
    bool editable;

    KeybindShortcut();

    ~KeybindShortcut();

    void updateActionsShortcut();

    static std::string keySymbolToString(Key key);
    static Key keySymbolFromString(const std::string& key);

    static std::string modifierToString(const KeyboardModifierEnum& key);
    static KeyboardModifierEnum modifierFromString(const std::string& key);

    static std::list<std::string> modifiersToStringList(const KeyboardModifiers& mods);
    static KeyboardModifiers modifiersFromStringList(const std::list<std::string>& mods);
};


NATRON_NAMESPACE_EXIT

#endif // NATRON_ENGINE_KEYBINDSHORTCUT_H
