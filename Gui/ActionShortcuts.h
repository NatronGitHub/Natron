#ifndef ACTIONSHORTCUTS_H
#define ACTIONSHORTCUTS_H

//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/**
 * @brief In this file all Natron's actions that can have their shortcut edited should be listed.
 **/
#include <map>
#include <list>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QString>
#include <QAction>

#define kShortcutGroupGlobal "Global"
#define kShortcutGroupNodegraph "NodeGraph"
#define kShortcutGroupCurveEditor "CurveEditor"
#define kShortcutGroupViewer "Viewer"
#define kShortcutGroupRoto "Roto"
#define kShortcutGroupTracking "Tracking"
#define kShortcutGroupPlayer "Player"
#define kShortcutGroupNodes "Nodes"

/////////GLOBAL SHORTCUTS
#define kShortcutIDActionNewProject "newProject"
#define kShortcutDescActionNewProject "Create a new project"

#define kShortcutIDActionOpenProject "openProject"
#define kShortcutDescActionOpenProject "Open a project"

#define kShortcutIDActionCloseProject "closeProject"
#define kShortcutDescActionCloseProject "Close project"

#define kShortcutIDActionSaveProject "saveProject"
#define kShortcutDescActionSaveProject "Save project"

#define kShortcutIDActionSaveAsProject "saveAsProject"
#define kShortcutDescActionSaveAsProject "Save as project"

#define kShortcutIDActionPreferences "preferences"
#define kShortcutDescActionPreferences "Preferences"

#define kShortcutIDActionQuit "quit"
#define kShortcutDescActionQuit "Quit application"

#define kShortcutIDActionNewProject "newProject"
#define kShortcutDescActionNewProject "Create a new project"

#define kShortcutIDActionProjectSettings "projectSettings"
#define kShortcutDescActionProjectSettings "Show project settings"

#define kShortcutIDActionShowOFXLog "showOFXLog"
#define kShortcutDescActionShowOFXLog "Show errors log"

#define kShortcutIDActionShowShortcutEditor "showShortcutEditor"
#define kShortcutDescActionShowShortcutEditor "Show shortcut editor"

#define kShortcutIDActionNewViewer "newViewer"
#define kShortcutDescActionNewViewer "New viewer"

#define kShortcutIDActionFullscreen "fullScreen"
#define kShortcutDescActionFullscreen "FullScreen"

#define kShortcutIDActionClearDiskCache "clearDiskCache"
#define kShortcutDescActionClearDiskCache "Clear disk cache"

#define kShortcutIDActionClearPlaybackCache "clearPlaybackCache"
#define kShortcutDescActionClearPlaybackCache "Clear playback cache"

#define kShortcutIDActionClearNodeCache "clearNodeCache"
#define kShortcutDescActionClearNodeCache "Clear per node cache"

#define kShortcutIDActionClearPluginsLoadCache "clearPluginsCache"
#define kShortcutDescActionClearPluginsLoadCache "Clear plug-ins load cache"

#define kShortcutIDActionClearAllCaches "clearAllCaches"
#define kShortcutDescActionClearAllCaches "Clear all caches"

#define kShortcutIDActionShowAbout "showAbout"
#define kShortcutDescActionShowAbout "About"

#define kShortcutIDActionRenderSelected "renderSelect"
#define kShortcutDescActionRenderSelected "Render selected writers"

#define kShortcutIDActionRenderAll "renderAll"
#define kShortcutDescActionRenderAll "Render all writers"

#define kShortcutIDActionConnectViewerToInput1 "connectViewerInput1"
#define kShortcutDescActionConnectViewerToInput1 "Connect viewer to input 1"

#define kShortcutIDActionConnectViewerToInput2 "connectViewerInput2"
#define kShortcutDescActionConnectViewerToInput2 "Connect viewer to input 2"

#define kShortcutIDActionConnectViewerToInput3 "connectViewerInput3"
#define kShortcutDescActionConnectViewerToInput3 "Connect viewer to input 3"

#define kShortcutIDActionConnectViewerToInput4 "connectViewerInput4"
#define kShortcutDescActionConnectViewerToInput4 "Connect viewer to input 4"

#define kShortcutIDActionConnectViewerToInput5 "connectViewerInput5"
#define kShortcutDescActionConnectViewerToInput5 "Connect viewer to input 5"

#define kShortcutIDActionConnectViewerToInput6 "connectViewerInput6"
#define kShortcutDescActionConnectViewerToInput6 "Connect viewer to input 6"

#define kShortcutIDActionConnectViewerToInput7 "connectViewerInput7"
#define kShortcutDescActionConnectViewerToInput7 "Connect viewer to input 7"

#define kShortcutIDActionConnectViewerToInput8 "connectViewerInput8"
#define kShortcutDescActionConnectViewerToInput8 "Connect viewer to input 8"

#define kShortcutIDActionConnectViewerToInput9 "connectViewerInput9"
#define kShortcutDescActionConnectViewerToInput9 "Connect viewer to input 9"

#define kShortcutIDActionConnectViewerToInput10 "connectViewerInput10"
#define kShortcutDescActionConnectViewerToInput10 "Connect viewer to input 10"

#define kShortcutIDActionShowPaneFullScreen "showPaneFullScreen"
#define kShortcutDescActionShowPaneFullScreen "Show pane full-screen"

#define kShortcutIDActionImportLayout "importLayout"
#define kShortcutDescActionImportLayout "Import layout"

#define kShortcutIDActionExportLayout "exportLayout"
#define kShortcutDescActionExportLayout "Export layout"

#define kShortcutIDActionDefaultLayout "restoreDefaultLayout"
#define kShortcutDescActionDefaultLayout "Restore default layout"

/////////VIEWER SHORTCUTS
#define kShortcutIDActionLuminance "luminance"
#define kShortcutDescActionLuminance "Luminance"

#define kShortcutIDActionR "channelR"
#define kShortcutDescActionR "Red"

#define kShortcutIDActionG "channelG"
#define kShortcutDescActionG "Green"

#define kShortcutIDActionB "channelB"
#define kShortcutDescActionB "Blue"

#define kShortcutIDActionA "channelA"
#define kShortcutDescActionA "Alpha"

#define kShortcutIDActionFitViewer "fitViewer"
#define kShortcutDescActionFitViewer "Fit image to viewer"

#define kShortcutIDActionClipEnabled "clipEnabled"
#define kShortcutDescActionClipEnabled "Enable clipping to project window"

#define kShortcutIDActionRefresh "refresh"
#define kShortcutDescActionRefresh "Refresh image"

#define kShortcutIDActionROIEnabled "userRoiEnabled"
#define kShortcutDescActionROIEnabled "Enable user roi"

#define kShortcutIDActionProxyEnabled "proxyEnabled"
#define kShortcutDescActionProxyEnabled "Enable proxy rendering"

#define kShortcutIDActionProxyLevel2 "proxy2"
#define kShortcutDescActionProxyLevel2 "2"

#define kShortcutIDActionProxyLevel4 "proxy4"
#define kShortcutDescActionProxyLevel4 "4"

#define kShortcutIDActionProxyLevel8 "proxy8"
#define kShortcutDescActionProxyLevel8 "8"

#define kShortcutIDActionProxyLevel16 "proxy16"
#define kShortcutDescActionProxyLevel16 "16"

#define kShortcutIDActionProxyLevel32 "proxy32"
#define kShortcutDescActionProxyLevel32 "32"

#define kShortcutIDActionZoomLevel100 "100%"
#define kShortcutDescActionZoomLevel100 "100%"

#define kShortcutIDActionHideOverlays "hideOverlays"
#define kShortcutDescActionHideOverlays "Show/Hide overlays"

#define kShortcutIDActionHidePlayer "hidePlayer"
#define kShortcutDescActionHidePlayer "Show/Hide player"

#define kShortcutIDActionHideTimeline "hideTimeline"
#define kShortcutDescActionHideTimeline "Show/Hide timeline"

#define kShortcutIDActionHideLeft "hideLeft"
#define kShortcutDescActionHideLeft "Show/Hide left toolbar"

#define kShortcutIDActionHideRight "hideRight"
#define kShortcutDescActionHideRight "Show/Hide right toolbar"

#define kShortcutIDActionHideTop "hideTop"
#define kShortcutDescActionHideTop "Show/Hide top toolbar"

#define kShortcutIDActionHideInfobar "hideInfos"
#define kShortcutDescActionHideInfobar "Show/Hide infos bar"

#define kShortcutIDActionHideAll "hideAll"
#define kShortcutDescActionHideAll "Hide all"

#define kShortcutIDActionShowAll "showAll"
#define kShortcutDescActionShowAll "Show all"

#define kShortcutIDMousePickColor "pick"
#define kShortcutDescMousePickColor "Pick a color"

#define kShortcutIDMouseRectanglePick "rectanglePick"
#define kShortcutDescMouseRectanglePick "Rectangle colour picker"

///////////PLAYER SHORTCUTS

#define kShortcutIDActionPlayerPrevious "prev"
#define kShortcutDescActionPlayerPrevious "Previous frame"

#define kShortcutIDActionPlayerNext "next"
#define kShortcutDescActionPlayerNext "Next frame"

#define kShortcutIDActionPlayerBackward "backward"
#define kShortcutDescActionPlayerBackward "Play backward (rewind)"

#define kShortcutIDActionPlayerForward "forward"
#define kShortcutDescActionPlayerForward "Play forward"

#define kShortcutIDActionPlayerStop "stop"
#define kShortcutDescActionPlayerStop "Stop"

#define kShortcutIDActionPlayerPrevIncr "prevIncr"
#define kShortcutDescActionPlayerPrevIncr "Go to current frame minus increment"

#define kShortcutIDActionPlayerNextIncr "nextIncr"
#define kShortcutDescActionPlayerNextIncr "Go to current frame plus increment"

#define kShortcutIDActionPlayerPrevKF "prevKF"
#define kShortcutDescActionPlayerPrevKF "Go to previous keyframe"

#define kShortcutIDActionPlayerNextKF "nextKF"
#define kShortcutDescActionPlayerNextKF "Go to next keyframe"

#define kShortcutIDActionPlayerFirst "first"
#define kShortcutDescActionPlayerFirst "Go to first frame"

#define kShortcutIDActionPlayerLast "last"
#define kShortcutDescActionPlayerLast "Go to last frame"


///////////ROTO SHORTCUTS
#define kShortcutIDActionRotoDelete "delete"
#define kShortcutDescActionRotoDelete "Delete element"

#define kShortcutIDActionRotoCloseBezier "closeBezier"
#define kShortcutDescActionRotoCloseBezier "Close bezier"

#define kShortcutIDActionRotoSelectAll "selectAll"
#define kShortcutDescActionRotoSelectAll "Select all"

#define kShortcutIDActionRotoSelectionTool "selectionTool"
#define kShortcutDescActionRotoSelectionTool "Switch to selection mode"

#define kShortcutIDActionRotoAddTool "addTool"
#define kShortcutDescActionRotoAddTool "Switch to add mode"

#define kShortcutIDActionRotoEditTool "editTool"
#define kShortcutDescActionRotoEditTool "Switch to edition mode"

#define kShortcutIDActionRotoNudgeLeft "nudgeLeft"
#define kShortcutDescActionRotoNudgeLeft "Move bezier on the left"

#define kShortcutIDActionRotoNudgeRight "nudgeRight"
#define kShortcutDescActionRotoNudgeRight "Move bezier on the right"

#define kShortcutIDActionRotoNudgeBottom "nudgeBottom"
#define kShortcutDescActionRotoNudgeBottom "Move bezier on the bottom"

#define kShortcutIDActionRotoNudgeTop "nudgeTop"
#define kShortcutDescActionRotoNudgeTop "Move bezier on the top"

#define kShortcutIDActionRotoSmooth "smooth"
#define kShortcutDescActionRotoSmooth "Smooth bezier"

#define kShortcutIDActionRotoCuspBezier "cusp"
#define kShortcutDescActionRotoCuspBezier "Cusp bezier"

#define kShortcutIDActionRotoRemoveFeather "rmvFeather"
#define kShortcutDescActionRotoRemoveFeather "Remove feather"

///////////TRACKING SHORTCUTS
#define kShortcutIDActionTrackingSelectAll "selectAll"
#define kShortcutDescActionTrackingSelectAll "Select All tracks"

#define kShortcutIDActionTrackingDelete "delete"
#define kShortcutDescActionTrackingDelete "Remove tracks"

#define kShortcutIDActionTrackingBackward "backward"
#define kShortcutDescActionTrackingBackward "Track backward"

#define kShortcutIDActionTrackingForward "forward"
#define kShortcutDescActionTrackingForward "Track forward"

#define kShortcutIDActionTrackingPrevious "prev"
#define kShortcutDescActionTrackingPrevious "Track to previous frame"

#define kShortcutIDActionTrackingNext "next"
#define kShortcutDescActionTrackingNext "Track to next frame"

#define kShortcutIDActionTrackingStop "stop"
#define kShortcutDescActionTrackingStop "Stop tracking"

///////////NODEGRAPH SHORTCUTS
#define kShortcutIDActionGraphCreateReader "createReader"
#define kShortcutDescActionGraphCreateReader "Create reader"

#define kShortcutIDActionGraphCreateWriter "createWriter"
#define kShortcutDescActionGraphCreateWriter "Create writer"

#define kShortcutIDActionGraphRearrangeNodes "rearrange"
#define kShortcutDescActionGraphRearrangeNodes "Rearrange nodes"

#define kShortcutIDActionGraphRemoveNodes "remove"
#define kShortcutDescActionGraphRemoveNodes "Remove nodes"

#define kShortcutIDActionGraphShowExpressions "displayExp"
#define kShortcutDescActionGraphShowExpressions "Display expressions links"

#define kShortcutIDActionGraphNavigateUpstream "navigateUp"
#define kShortcutDescActionGraphNavigateUpstream "Navigate tree upward"

#define kShortcutIDActionGraphNavigateDownstream "navigateDown"
#define kShortcutDescActionGraphNavigateDownstram "Navigate tree downward"

#define kShortcutIDActionGraphSelectUp "selUp"
#define kShortcutDescActionGraphSelectUp "Select tree upward"

#define kShortcutIDActionGraphSelectDown "selDown"
#define kShortcutDescActionGraphSelectDown "Select tree downward"

#define kShortcutIDActionGraphSelectAll "selectAll"
#define kShortcutDescActionGraphSelectAll "Select all nodes"

#define kShortcutIDActionGraphSelectAllVisible "selectAllVisible"
#define kShortcutDescActionGraphSelectAllVisible "Select all nodes visible"

#define kShortcutIDActionGraphEnableHints "hints"
#define kShortcutDescActionGraphEnableHints "Enable connection hints"

#define kShortcutIDActionGraphSwitchInputs "switchInputs"
#define kShortcutDescActionGraphSwitchInputs "Switch input 1 and 2"

#define kShortcutIDActionGraphCopy "copy"
#define kShortcutDescActionGraphCopy "Copy nodes"

#define kShortcutIDActionGraphPaste "paste"
#define kShortcutDescActionGraphPaste "Paste nodes"

#define kShortcutIDActionGraphClone "clone"
#define kShortcutDescActionGraphClone "Clone nodes"

#define kShortcutIDActionGraphDeclone "declone"
#define kShortcutDescActionGraphDeclone "Declone nodes"

#define kShortcutIDActionGraphCut "cut"
#define kShortcutDescActionGraphCut "Cut nodes"

#define kShortcutIDActionGraphDuplicate "duplicate"
#define kShortcutDescActionGraphDuplicate "Duplicate nodes"

#define kShortcutIDActionGraphDisableNodes "disable"
#define kShortcutDescActionGraphDisableNodes "Disable nodes"

#define kShortcutIDActionGraphToggleAutoPreview "toggleAutoPreview"
#define kShortcutDescActionGraphToggleAutoPreview "Auto previews"

#define kShortcutIDActionGraphTogglePreview "togglePreview"
#define kShortcutDescActionGraphTogglePreview "Toggle preview images"

#define kShortcutIDActionGraphForcePreview "preview"
#define kShortcutDescActionGraphForcePreview "Refresh preview images"

#define kShortcutIDActionGraphShowCacheSize "cacheSize"
#define kShortcutDescActionGraphShowCacheSize "Diplay cache memory consumption"


#define kShortcutIDActionGraphFrameNodes "frameNodes"
#define kShortcutDescActionGraphFrameNodes "Center on all nodes"

#define kShortcutIDActionGraphFindNode "findNode"
#define kShortcutDescActionGraphFindNode "Find"

#define kShortcutIDActionGraphRenameNode "renameNode"
#define kShortcutDescActionGraphRenameNode "Rename node"

///////////CURVEEDITOR SHORTCUTS
#define kShortcutIDActionCurveEditorRemoveKeys "remove"
#define kShortcutDescActionCurveEditorRemoveKeys "Delete keyframes"

#define kShortcutIDActionCurveEditorConstant "constant"
#define kShortcutDescActionCurveEditorConstant "Constant interpolation"

#define kShortcutIDActionCurveEditorLinear "linear"
#define kShortcutDescActionCurveEditorLinear "Linear interpolation"

#define kShortcutIDActionCurveEditorSmooth "smooth"
#define kShortcutDescActionCurveEditorSmooth "Smooth interpolation"

#define kShortcutIDActionCurveEditorCatmullrom "catmullrom"
#define kShortcutDescActionCurveEditorCatmullrom "Catmull-rom interpolation"

#define kShortcutIDActionCurveEditorCubic "cubic"
#define kShortcutDescActionCurveEditorCubic "Cubic interpolation"

#define kShortcutIDActionCurveEditorHorizontal "horiz"
#define kShortcutDescActionCurveEditorHorizontal "Horizontal interpolation"

#define kShortcutIDActionCurveEditorBreak "break"
#define kShortcutDescActionCurveEditorBreak "Break curvature"

#define kShortcutIDActionCurveEditorSelectAll "selectAll"
#define kShortcutDescActionCurveEditorSelectAll "Select all keyframes"

#define kShortcutIDActionCurveEditorCenter "center"
#define kShortcutDescActionCurveEditorCenter "Center on curve"

#define kShortcutIDActionCurveEditorCopy "copy"
#define kShortcutDescActionCurveEditorCopy "Copy keyframes"

#define kShortcutIDActionCurveEditorPaste "paste"
#define kShortcutDescActionCurveEditorPaste "Paste keyframes"

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
    QString description; //< the description that will be in the shortcut editor
    Qt::KeyboardModifiers modifiers; //< the keyboard modifiers that must be held down during the action
    Qt::KeyboardModifiers defaultModifiers; //< the default keyboard modifiers

    BoundAction()
        : editable(true)
    {
    }

    virtual ~BoundAction()
    {
    }
};

class KeyBoundAction
    : public BoundAction
{
public:

    Qt::Key currentShortcut; //< the actual shortcut for the keybind
    Qt::Key defaultShortcut; //< the default shortcut proposed by the dev team
    std::list<QAction*> actions; //< list of actions using this shortcut

    KeyBoundAction()
        : BoundAction()
    {
    }

    virtual ~KeyBoundAction()
    {
    }

    void updateActionsShortcut()
    {
        for (std::list<QAction*>::iterator it = actions.begin(); it != actions.end(); ++it) {
            (*it)->setShortcut( makeKeySequence(modifiers, currentShortcut) );
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

class ActionWithShortcut
: public QAction
{
    QString _group;
    QString _actionID;
    
public:
    
    ActionWithShortcut(const QString & group,
                       const QString & actionID,
                       const QString & actionDescription,
                       QObject* parent);
    
    virtual ~ActionWithShortcut();};

#endif // ACTIONSHORTCUTS_H
