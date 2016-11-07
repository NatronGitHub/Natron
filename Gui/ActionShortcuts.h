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
#define kShortcutGroupAnimationModule "AnimatioEditor"
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

#define kShortcutIDActionShowCacheReport "showCacheReport"
#define kShortcutDescActionShowCacheReport "Show Cache Report"

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

///////////NODEGRAPH SHORTCUTS

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
#define kShortcutIDActionAnimationModuleRemoveKeys "remove"
#define kShortcutDescActionAnimationModuleRemoveKeys "Delete Keyframes"

#define kShortcutIDActionAnimationModuleConstant "constant"
#define kShortcutDescActionAnimationModuleConstant "Constant Interpolation"

#define kShortcutIDActionAnimationModuleLinear "linear"
#define kShortcutDescActionAnimationModuleLinear "Linear Interpolation"

#define kShortcutIDActionAnimationModuleSmooth "smooth"
#define kShortcutDescActionAnimationModuleSmooth "Smooth Interpolation"

#define kShortcutIDActionAnimationModuleCatmullrom "catmullrom"
#define kShortcutDescActionAnimationModuleCatmullrom "Catmull-Rom Interpolation"

#define kShortcutIDActionAnimationModuleCubic "cubic"
#define kShortcutDescActionAnimationModuleCubic "Cubic Interpolation"

#define kShortcutIDActionAnimationModuleHorizontal "horiz"
#define kShortcutDescActionAnimationModuleHorizontal "Horizontal Interpolation"

#define kShortcutIDActionAnimationModuleBreak "break"
#define kShortcutDescActionAnimationModuleBreak "Break Tangents"

#define kShortcutIDActionAnimationModuleSelectAll "selectAll"
#define kShortcutDescActionAnimationModuleSelectAll "Select All"

#define kShortcutIDActionAnimationModuleCenterAll "centerAll"
#define kShortcutDescActionAnimationModuleCenterAll "Center on All Items"

#define kShortcutIDActionAnimationModuleCenter "center"
#define kShortcutDescActionAnimationModuleCenter "Center on Selected Items"

#define kShortcutIDActionAnimationModuleCopy "copy"
#define kShortcutDescActionAnimationModuleCopy "Copy Selected Keyframes"

#define kShortcutIDActionAnimationModulePasteKeyframes "pastekeyframes"
#define kShortcutDescActionAnimationModulePasteKeyframes "Paste Keyframes"

#define kShortcutIDActionAnimationModulePasteKeyframesAbsolute "pastekeyframesAbs"
#define kShortcutDescActionAnimationModulePasteKeyframesAbsolute "Paste Keyframes Absolute"

#define kShortcutIDActionAnimationModuleStackView "stackAnimView"
#define kShortcutDescActionAnimationModuleStackView "Stack/un-stack Curve Editor and Dope Sheet"


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

NATRON_NAMESPACE_ENTER;

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

NATRON_NAMESPACE_EXIT;

#endif // ACTIONSHORTCUTS_H
