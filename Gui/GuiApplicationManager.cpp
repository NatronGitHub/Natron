/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://natrongithub.github.io/>,
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

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "GuiApplicationManager.h"
#include "GuiApplicationManagerPrivate.h"

#include <stdexcept>

CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QtCore/QtGlobal> // for Q_OS_*
#include <QtCore/QDebug>
#include <QtCore/QSettings>
#include <QPixmapCache>
#include <QApplication>
#include <QFontDatabase>
#include <QtConcurrentRun> // QtCore on Qt4, QtConcurrent on Qt5
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#ifdef DEBUG
#include "Global/FloatingPointExceptions.h"
#endif

#include "Engine/Settings.h"
#include "Engine/EffectInstance.h" // PLUGINID_OFX_*
#include "Engine/PluginActionShortcut.h"
#include "Gui/QtEnumConvert.h"
#include "Gui/GuiAppInstance.h"
#include "Gui/Gui.h"
#include "Gui/GuiDefines.h"
#include "Gui/KnobGuiFactory.h"
#include "Gui/SplashScreen.h"
#include "Gui/PreviewThread.h"
#include "Gui/DocumentationManager.h"

//All fixed sizes were calculated for a 96 dpi screen
#ifndef Q_OS_MAC
#define NATRON_PIXELS_FOR_DPI_DEFAULT 96.
#else
#define NATRON_PIXELS_FOR_DPI_DEFAULT 72.
#endif

NATRON_NAMESPACE_ENTER


GuiApplicationManager::GuiApplicationManager()
    : AppManager()
    , _imp( new GuiApplicationManagerPrivate(this) )
{
}

GuiApplicationManager::~GuiApplicationManager()
{
  
    _imp->previewRenderThread.quitThread(false);
}

void
GuiApplicationManager::getIcon(PixmapEnum e,
                               QPixmap* pix) const
{
    int iconSet = appPTR->getCurrentSettings()->getIconsBlackAndWhite() ? 2 : 3;
    std::string iconSetStr = QString::number(iconSet).toStdString();

    if ( !QPixmapCache::find(QString::number(e), pix) ) {
        std::string path;
        switch (e) {
        case NATRON_PIXMAP_PLAYER_PREVIOUS:
            path = NATRON_IMAGES_PATH "back1.png";
            break;
        case NATRON_PIXMAP_PLAYER_FIRST_FRAME:
            path = NATRON_IMAGES_PATH "firstFrame.png";
            break;
        case NATRON_PIXMAP_PLAYER_NEXT:
            path = NATRON_IMAGES_PATH "forward1.png";
            break;
        case NATRON_PIXMAP_PLAYER_LAST_FRAME:
            path = NATRON_IMAGES_PATH "lastFrame.png";
            break;
        case NATRON_PIXMAP_PLAYER_NEXT_INCR:
            path = NATRON_IMAGES_PATH "nextIncr.png";
            break;
        case NATRON_PIXMAP_PLAYER_NEXT_KEY:
            path = NATRON_IMAGES_PATH "nextKF.png";
            break;
        case NATRON_PIXMAP_PLAYER_PREVIOUS_INCR:
            path = NATRON_IMAGES_PATH "previousIncr.png";
            break;
        case NATRON_PIXMAP_PLAYER_PREVIOUS_KEY:
            path = NATRON_IMAGES_PATH "prevKF.png";
            break;
        case NATRON_PIXMAP_ADD_KEYFRAME:
            path = NATRON_IMAGES_PATH "addKF.png";
            break;
        case NATRON_PIXMAP_REMOVE_KEYFRAME:
            path = NATRON_IMAGES_PATH "removeKF.png";
            break;
        case NATRON_PIXMAP_PLAYER_REWIND_ENABLED:
            path = NATRON_IMAGES_PATH "rewind_enabled.png";
            break;
        case NATRON_PIXMAP_PLAYER_REWIND_DISABLED:
            path = NATRON_IMAGES_PATH "rewind.png";
            break;
        case NATRON_PIXMAP_PLAYER_PLAY_ENABLED:
            path = NATRON_IMAGES_PATH "play_enabled.png";
            break;
        case NATRON_PIXMAP_PLAYER_PLAY_DISABLED:
            path = NATRON_IMAGES_PATH "play.png";
            break;
        case NATRON_PIXMAP_PLAYER_STOP_ENABLED:
            path = NATRON_IMAGES_PATH "stopEnabled.png";
            break;
        case NATRON_PIXMAP_PLAYER_STOP_DISABLED:
            path = NATRON_IMAGES_PATH "stopDisabled.png";
            break;
        case NATRON_PIXMAP_PLAYER_PAUSE_ENABLED:
            path = NATRON_IMAGES_PATH "pauseEnabled.png";
            break;
        case NATRON_PIXMAP_PLAYER_PAUSE_DISABLED:
            path = NATRON_IMAGES_PATH "pauseDisabled.png";
            break;

        case NATRON_PIXMAP_PLAYER_LOOP_MODE:
            path = NATRON_IMAGES_PATH "loopmode.png";
            break;
        case NATRON_PIXMAP_PLAYER_BOUNCE:
            path = NATRON_IMAGES_PATH "bounce.png";
            break;
        case NATRON_PIXMAP_PLAYER_PLAY_ONCE:
            path = NATRON_IMAGES_PATH "playOnce.png";
            break;
        case NATRON_PIXMAP_PLAYER_TIMELINE_IN:
            path = NATRON_IMAGES_PATH "timelineIn.png";
            break;
        case NATRON_PIXMAP_PLAYER_TIMELINE_OUT:
            path = NATRON_IMAGES_PATH "timelineOut.png";
            break;
        case NATRON_PIXMAP_MAXIMIZE_WIDGET:
            path = NATRON_IMAGES_PATH "maximize.png";
            break;
        case NATRON_PIXMAP_MINIMIZE_WIDGET:
            path = NATRON_IMAGES_PATH "minimize.png";
            break;
        case NATRON_PIXMAP_CLOSE_WIDGET:
            path = NATRON_IMAGES_PATH "close.png";
            break;
        case NATRON_PIXMAP_CLOSE_PANEL:
            path = NATRON_IMAGES_PATH "closePanel.png";
            break;
        case NATRON_PIXMAP_HIDE_UNMODIFIED:
            path = NATRON_IMAGES_PATH "hide_unmodified.png";
            break;
        case NATRON_PIXMAP_UNHIDE_UNMODIFIED:
            path = NATRON_IMAGES_PATH "unhide_unmodified.png";
            break;
        case NATRON_PIXMAP_HELP_WIDGET:
            path = NATRON_IMAGES_PATH "help.png";
            break;
        case NATRON_PIXMAP_GROUPBOX_FOLDED:
            path = NATRON_IMAGES_PATH "groupbox_folded.png";
            break;
        case NATRON_PIXMAP_GROUPBOX_UNFOLDED:
            path = NATRON_IMAGES_PATH "groupbox_unfolded.png";
            break;
        case NATRON_PIXMAP_UNDO:
            path = NATRON_IMAGES_PATH "undo.png";
            break;
        case NATRON_PIXMAP_REDO:
            path = NATRON_IMAGES_PATH "redo.png";
            break;
        case NATRON_PIXMAP_UNDO_GRAYSCALE:
            path = NATRON_IMAGES_PATH "undo_grayscale.png";
            break;
        case NATRON_PIXMAP_REDO_GRAYSCALE:
            path = NATRON_IMAGES_PATH "redo_grayscale.png";
            break;
        case NATRON_PIXMAP_RESTORE_DEFAULTS_ENABLED:
            path = NATRON_IMAGES_PATH "restoreDefaultEnabled.png";
            break;
        case NATRON_PIXMAP_RESTORE_DEFAULTS_DISABLED:
            path = NATRON_IMAGES_PATH "restoreDefaultDisabled.png";
            break;
        case NATRON_PIXMAP_TAB_WIDGET_LAYOUT_BUTTON:
            path = NATRON_IMAGES_PATH "layout.png";
            break;
        case NATRON_PIXMAP_TAB_WIDGET_LAYOUT_BUTTON_ANCHOR:
            path = NATRON_IMAGES_PATH "layout_anchor.png";
            break;
        case NATRON_PIXMAP_TAB_WIDGET_SPLIT_HORIZONTALLY:
            path = NATRON_IMAGES_PATH "splitHorizontally.png";
            break;
        case NATRON_PIXMAP_TAB_WIDGET_SPLIT_VERTICALLY:
            path = NATRON_IMAGES_PATH "splitVertically.png";
            break;
        case NATRON_PIXMAP_VIEWER_CENTER:
            path = NATRON_IMAGES_PATH "centerViewer.png";
            break;
        case NATRON_PIXMAP_VIEWER_FULL_FRAME_OFF:
            path = NATRON_IMAGES_PATH "fullFrameOff.png";
            break;
        case NATRON_PIXMAP_VIEWER_FULL_FRAME_ON:
            path = NATRON_IMAGES_PATH "fullFrameOn.png";
            break;
        case NATRON_PIXMAP_VIEWER_CLIP_TO_PROJECT_ENABLED:
            path = NATRON_IMAGES_PATH "cliptoprojectEnabled.png";
            break;
        case NATRON_PIXMAP_VIEWER_CLIP_TO_PROJECT_DISABLED:
            path = NATRON_IMAGES_PATH "cliptoprojectDisabled.png";
            break;
        case NATRON_PIXMAP_VIEWER_REFRESH:
            path = NATRON_IMAGES_PATH "refresh.png";
            break;
        case NATRON_PIXMAP_VIEWER_REFRESH_ACTIVE:
            path = NATRON_IMAGES_PATH "refreshActive.png";
            break;
        case NATRON_PIXMAP_VIEWER_ROI_ENABLED:
            path = NATRON_IMAGES_PATH "viewer_roiEnabled.png";
            break;
        case NATRON_PIXMAP_VIEWER_ROI_DISABLED:
            path = NATRON_IMAGES_PATH "viewer_roiDisabled.png";
            break;
        case NATRON_PIXMAP_VIEWER_RENDER_SCALE:
            path = NATRON_IMAGES_PATH "renderScale.png";
            break;
        case NATRON_PIXMAP_VIEWER_RENDER_SCALE_CHECKED:
            path = NATRON_IMAGES_PATH "renderScale_checked.png";
            break;
        case NATRON_PIXMAP_VIEWER_AUTOCONTRAST_ENABLED:
            path = NATRON_IMAGES_PATH "AutoContrastON.png";
            break;
        case NATRON_PIXMAP_VIEWER_AUTOCONTRAST_DISABLED:
            path = NATRON_IMAGES_PATH "AutoContrast.png";
            break;
        case NATRON_PIXMAP_OPEN_FILE:
            path = NATRON_IMAGES_PATH "open-file.png";
            break;
        case NATRON_PIXMAP_COLORWHEEL:
            path = NATRON_IMAGES_PATH "colorwheel.png";
            break;
        case NATRON_PIXMAP_OVERLAY:
            path = NATRON_IMAGES_PATH "colorwheel_overlay.png";
            break;
        case NATRON_PIXMAP_ROTO_MERGE:
            path = NATRON_IMAGES_PATH "roto_merge.png";
            break;
        case NATRON_PIXMAP_COLOR_PICKER:
            path = NATRON_IMAGES_PATH "color_picker.png";
            break;


        case NATRON_PIXMAP_IO_GROUPING:
            path = NATRON_IMAGES_PATH "GroupingIcons/Set" + iconSetStr + "/image_grouping_" + iconSetStr + ".png";
            break;
        case NATRON_PIXMAP_3D_GROUPING:
            path = NATRON_IMAGES_PATH "GroupingIcons/Set" + iconSetStr + "/3D_grouping_" + iconSetStr + ".png";
            break;
        case NATRON_PIXMAP_CHANNEL_GROUPING:
            path = NATRON_IMAGES_PATH "GroupingIcons/Set" + iconSetStr + "/channel_grouping_" + iconSetStr + ".png";
            break;
        case NATRON_PIXMAP_MERGE_GROUPING:
            path = NATRON_IMAGES_PATH "GroupingIcons/Set" + iconSetStr + "/merge_grouping_" + iconSetStr + ".png";
            break;
        case NATRON_PIXMAP_COLOR_GROUPING:
            path = NATRON_IMAGES_PATH "GroupingIcons/Set" + iconSetStr + "/color_grouping_" + iconSetStr + ".png";
            break;
        case NATRON_PIXMAP_TRANSFORM_GROUPING:
            path = NATRON_IMAGES_PATH "GroupingIcons/Set" + iconSetStr + "/transform_grouping_" + iconSetStr + ".png";
            break;
        case NATRON_PIXMAP_DEEP_GROUPING:
            path = NATRON_IMAGES_PATH "GroupingIcons/Set" + iconSetStr + "/deep_grouping_" + iconSetStr + ".png";
            break;
        case NATRON_PIXMAP_FILTER_GROUPING:
            path = NATRON_IMAGES_PATH "GroupingIcons/Set" + iconSetStr + "/filter_grouping_" + iconSetStr + ".png";
            break;
        case NATRON_PIXMAP_MULTIVIEW_GROUPING:
            path = NATRON_IMAGES_PATH "GroupingIcons/Set" + iconSetStr + "/multiview_grouping_" + iconSetStr + ".png";
            break;
        case NATRON_PIXMAP_MISC_GROUPING:
            path = NATRON_IMAGES_PATH "GroupingIcons/Set" + iconSetStr + "/misc_grouping_" + iconSetStr + ".png";
            break;
        case NATRON_PIXMAP_OTHER_PLUGINS:
            path = NATRON_IMAGES_PATH "GroupingIcons/Set" + iconSetStr + "/other_grouping_" + iconSetStr + ".png";
            break;
        case NATRON_PIXMAP_TOOLSETS_GROUPING:
            path = NATRON_IMAGES_PATH "GroupingIcons/Set" + iconSetStr + "/toolsets_grouping_" + iconSetStr + ".png";
            break;
        case NATRON_PIXMAP_KEYER_GROUPING:
            path = NATRON_IMAGES_PATH "GroupingIcons/Set" + iconSetStr + "/keyer_grouping_" + iconSetStr + ".png";
            break;
        case NATRON_PIXMAP_TIME_GROUPING:
            path = NATRON_IMAGES_PATH "GroupingIcons/Set" + iconSetStr + "/time_grouping_" + iconSetStr + ".png";
            break;
        case NATRON_PIXMAP_PAINT_GROUPING:
            path = NATRON_IMAGES_PATH "GroupingIcons/Set" + iconSetStr + "/paint_grouping_" + iconSetStr + ".png";
            break;


        case NATRON_PIXMAP_OPEN_EFFECTS_GROUPING:
            path = NATRON_IMAGES_PATH "openeffects.png";
            break;
        case NATRON_PIXMAP_COMBOBOX:
            path = NATRON_IMAGES_PATH "combobox.png";
            break;
        case NATRON_PIXMAP_COMBOBOX_PRESSED:
            path = NATRON_IMAGES_PATH "pressed_combobox.png";
            break;
        case NATRON_PIXMAP_READ_IMAGE:
            path = NATRON_IMAGES_PATH "readImage.png";
            break;
        case NATRON_PIXMAP_WRITE_IMAGE:
            path = NATRON_IMAGES_PATH "writeImage.png";
            break;

        case NATRON_PIXMAP_APP_ICON:
            path = NATRON_APPLICATION_ICON_PATH;
            break;
        case NATRON_PIXMAP_INVERTED:
            path = NATRON_IMAGES_PATH "inverted.png";
            break;
        case NATRON_PIXMAP_UNINVERTED:
            path = NATRON_IMAGES_PATH "uninverted.png";
            break;
        case NATRON_PIXMAP_VISIBLE:
            path = NATRON_IMAGES_PATH "visible.png";
            break;
        case NATRON_PIXMAP_UNVISIBLE:
            path = NATRON_IMAGES_PATH "unvisible.png";
            break;
        case NATRON_PIXMAP_LOCKED:
            path = NATRON_IMAGES_PATH "locked.png";
            break;
        case NATRON_PIXMAP_UNLOCKED:
            path = NATRON_IMAGES_PATH "unlocked.png";
            break;
        case NATRON_PIXMAP_LAYER:
            path = NATRON_IMAGES_PATH "layer.png";
            break;
        case NATRON_PIXMAP_BEZIER:
            path = NATRON_IMAGES_PATH "bezier.png";
            break;
        case NATRON_PIXMAP_CURVE:
            path = NATRON_IMAGES_PATH "curve.png";
            break;
        case NATRON_PIXMAP_BEZIER_32:
            path = NATRON_IMAGES_PATH "bezier32.png";
            break;
        case NATRON_PIXMAP_PENCIL:
            path = NATRON_IMAGES_PATH "rotoToolPencil.png";
            break;
        case NATRON_PIXMAP_ELLIPSE:
            path = NATRON_IMAGES_PATH "ellipse.png";
            break;
        case NATRON_PIXMAP_RECTANGLE:
            path = NATRON_IMAGES_PATH "rectangle.png";
            break;
        case NATRON_PIXMAP_ADD_POINTS:
            path = NATRON_IMAGES_PATH "addPoints.png";
            break;
        case NATRON_PIXMAP_REMOVE_POINTS:
            path = NATRON_IMAGES_PATH "removePoints.png";
            break;
        case NATRON_PIXMAP_REMOVE_FEATHER:
            path = NATRON_IMAGES_PATH "removeFeather.png";
            break;
        case NATRON_PIXMAP_CUSP_POINTS:
            path = NATRON_IMAGES_PATH "cuspPoints.png";
            break;
        case NATRON_PIXMAP_SMOOTH_POINTS:
            path = NATRON_IMAGES_PATH "smoothPoints.png";
            break;
        case NATRON_PIXMAP_OPEN_CLOSE_CURVE:
            path = NATRON_IMAGES_PATH "openCloseCurve.png";
            break;
        case NATRON_PIXMAP_SELECT_ALL:
            path = NATRON_IMAGES_PATH "cursor.png";
            break;
        case NATRON_PIXMAP_SELECT_POINTS:
            path = NATRON_IMAGES_PATH "selectPoints.png";
            break;
        case NATRON_PIXMAP_SELECT_FEATHER:
            path = NATRON_IMAGES_PATH "selectFeather.png";
            break;
        case NATRON_PIXMAP_SELECT_CURVES:
            path = NATRON_IMAGES_PATH "selectCurves.png";
            break;
        case NATRON_PIXMAP_AUTO_KEYING_ENABLED:
            path = NATRON_IMAGES_PATH "autoKeyingEnabled.png";
            break;
        case NATRON_PIXMAP_AUTO_KEYING_DISABLED:
            path = NATRON_IMAGES_PATH "autoKeyingDisabled.png";
            break;
        case NATRON_PIXMAP_STICKY_SELECTION_ENABLED:
            path = NATRON_IMAGES_PATH "stickySelectionEnabled.png";
            break;
        case NATRON_PIXMAP_STICKY_SELECTION_DISABLED:
            path = NATRON_IMAGES_PATH "stickySelectionDisabled.png";
            break;
        case NATRON_PIXMAP_FEATHER_LINK_ENABLED:
            path = NATRON_IMAGES_PATH "featherLinkEnabled.png";
            break;
        case NATRON_PIXMAP_FEATHER_LINK_DISABLED:
            path = NATRON_IMAGES_PATH "featherLinkDisabled.png";
            break;
        case NATRON_PIXMAP_FEATHER_VISIBLE:
            path = NATRON_IMAGES_PATH "featherEnabled.png";
            break;
        case NATRON_PIXMAP_FEATHER_UNVISIBLE:
            path = NATRON_IMAGES_PATH "featherDisabled.png";
            break;
        case NATRON_PIXMAP_RIPPLE_EDIT_ENABLED:
            path = NATRON_IMAGES_PATH "rippleEditEnabled.png";
            break;
        case NATRON_PIXMAP_RIPPLE_EDIT_DISABLED:
            path = NATRON_IMAGES_PATH "rippleEditDisabled.png";
            break;
        case NATRON_PIXMAP_BOLD_CHECKED:
            path = NATRON_IMAGES_PATH "bold_checked.png";
            break;
        case NATRON_PIXMAP_BOLD_UNCHECKED:
            path = NATRON_IMAGES_PATH "bold_unchecked.png";
            break;
        case NATRON_PIXMAP_ITALIC_UNCHECKED:
            path = NATRON_IMAGES_PATH "italic_unchecked.png";
            break;
        case NATRON_PIXMAP_ITALIC_CHECKED:
            path = NATRON_IMAGES_PATH "italic_checked.png";
            break;
        case NATRON_PIXMAP_CLEAR_ALL_ANIMATION:
            path = NATRON_IMAGES_PATH "clearAnimation.png";
            break;
        case NATRON_PIXMAP_CLEAR_BACKWARD_ANIMATION:
            path = NATRON_IMAGES_PATH "clearAnimationBw.png";
            break;
        case NATRON_PIXMAP_CLEAR_FORWARD_ANIMATION:
            path = NATRON_IMAGES_PATH "clearAnimationFw.png";
            break;
        case NATRON_PIXMAP_UPDATE_VIEWER_ENABLED:
            path = NATRON_IMAGES_PATH "updateViewerEnabled.png";
            break;
        case NATRON_PIXMAP_UPDATE_VIEWER_DISABLED:
            path = NATRON_IMAGES_PATH "updateViewerDisabled.png";
            break;
        case NATRON_PIXMAP_SETTINGS:
            path = NATRON_IMAGES_PATH "settings.png";
            break;
        case NATRON_PIXMAP_FREEZE_ENABLED:
            path = NATRON_IMAGES_PATH "turbo_on.png";
            break;
        case NATRON_PIXMAP_FREEZE_DISABLED:
            path = NATRON_IMAGES_PATH "turbo_off.png";
            break;
        case NATRON_PIXMAP_VIEWER_ICON:
            path = NATRON_IMAGES_PATH "viewer_icon.png";
            break;
        case NATRON_PIXMAP_VIEWER_CHECKERBOARD_ENABLED:
            path = NATRON_IMAGES_PATH "checkerboard_on.png";
            break;
        case NATRON_PIXMAP_VIEWER_CHECKERBOARD_DISABLED:
            path = NATRON_IMAGES_PATH "checkerboard_off.png";
            break;
        case NATRON_PIXMAP_VIEWER_ZEBRA_ENABLED:
            path = NATRON_IMAGES_PATH "zebra_on.png";
            break;
        case NATRON_PIXMAP_VIEWER_ZEBRA_DISABLED:
            path = NATRON_IMAGES_PATH "zebra_off.png";
            break;
        case NATRON_PIXMAP_VIEWER_GAMMA_ENABLED:
            path = NATRON_IMAGES_PATH "gammaON.png";
            break;
        case NATRON_PIXMAP_VIEWER_GAMMA_DISABLED:
            path = NATRON_IMAGES_PATH "gammaOFF.png";
            break;
        case NATRON_PIXMAP_VIEWER_GAIN_ENABLED:
            path = NATRON_IMAGES_PATH "expoON.png";
            break;
        case NATRON_PIXMAP_VIEWER_GAIN_DISABLED:
            path = NATRON_IMAGES_PATH "expoOFF.png";
            break;
        case NATRON_PIXMAP_ADD_TRACK:
            path = NATRON_IMAGES_PATH "addTrack.png";
            break;
        case NATRON_PIXMAP_ADD_USER_KEY:
            path = NATRON_IMAGES_PATH "addUserKey.png";
            break;
        case NATRON_PIXMAP_REMOVE_USER_KEY:
            path = NATRON_IMAGES_PATH "removeUserKey.png";
            break;
        case NATRON_PIXMAP_PREV_USER_KEY:
            path = NATRON_IMAGES_PATH "prevUserKey.png";
            break;
        case NATRON_PIXMAP_NEXT_USER_KEY:
            path = NATRON_IMAGES_PATH "nextUserKey.png";
            break;
        case NATRON_PIXMAP_TRACK_RANGE:
            path = NATRON_IMAGES_PATH "trackRange.png";
            break;
        case NATRON_PIXMAP_TRACK_ALL_KEYS:
            path = NATRON_IMAGES_PATH "trackAllKeyframes.png";
            break;
        case NATRON_PIXMAP_TRACK_CURRENT_KEY:
            path = NATRON_IMAGES_PATH "trackCurrentKeyframe.png";
            break;
        case NATRON_PIXMAP_TRACK_BACKWARD_OFF:
            path = NATRON_IMAGES_PATH "trackBackwardOff.png";
            break;
        case NATRON_PIXMAP_TRACK_BACKWARD_ON:
            path = NATRON_IMAGES_PATH "trackBackwardOn.png";
            break;
        case NATRON_PIXMAP_TRACK_FORWARD_OFF:
            path = NATRON_IMAGES_PATH "trackForwardOff.png";
            break;
        case NATRON_PIXMAP_TRACK_FORWARD_ON:
            path = NATRON_IMAGES_PATH "trackForwardOn.png";
            break;
        case NATRON_PIXMAP_TRACK_PREVIOUS:
            path = NATRON_IMAGES_PATH "trackPrev.png";
            break;
        case NATRON_PIXMAP_TRACK_NEXT:
            path = NATRON_IMAGES_PATH "trackNext.png";
            break;
        case NATRON_PIXMAP_RESET_TRACK_OFFSET:
            path = NATRON_IMAGES_PATH "resetTrackOffset.png";
            break;
        case NATRON_PIXMAP_HIDE_TRACK_ERROR:
            path = NATRON_IMAGES_PATH "hideTrackError.png";
            break;
        case NATRON_PIXMAP_SHOW_TRACK_ERROR:
            path = NATRON_IMAGES_PATH "showTrackError.png";
            break;
        case NATRON_PIXMAP_CREATE_USER_KEY_ON_MOVE_ON:
            path = NATRON_IMAGES_PATH "createKeyOnMoveOn.png";
            break;
        case NATRON_PIXMAP_CREATE_USER_KEY_ON_MOVE_OFF:
            path = NATRON_IMAGES_PATH "createKeyOnMoveOff.png";
            break;
        case NATRON_PIXMAP_RESET_USER_KEYS:
            path = NATRON_IMAGES_PATH "removeUserKeys.png";
            break;
        case NATRON_PIXMAP_CENTER_VIEWER_ON_TRACK:
            path = NATRON_IMAGES_PATH "centerOnTrack.png";
            break;
        case NATRON_PIXMAP_SCRIPT_CLEAR_OUTPUT:
            path = NATRON_IMAGES_PATH "clearOutput.png";
            break;
        case NATRON_PIXMAP_SCRIPT_EXEC_SCRIPT:
            path = NATRON_IMAGES_PATH "execScript.png";
            break;
        case NATRON_PIXMAP_SCRIPT_LOAD_EXEC_SCRIPT:
            path = NATRON_IMAGES_PATH "loadAndExecScript.png";
            break;
        case NATRON_PIXMAP_SCRIPT_LOAD_SCRIPT:
            path = NATRON_IMAGES_PATH "loadScript.png";
            break;
        case NATRON_PIXMAP_SCRIPT_NEXT_SCRIPT:
            path = NATRON_IMAGES_PATH "nextScript.png";
            break;
        case NATRON_PIXMAP_SCRIPT_OUTPUT_PANE_ACTIVATED:
            path = NATRON_IMAGES_PATH "outputPanelActivated.png";
            break;
        case NATRON_PIXMAP_SCRIPT_OUTPUT_PANE_DEACTIVATED:
            path = NATRON_IMAGES_PATH "outputPanelDeactivated.png";
            break;
        case NATRON_PIXMAP_SCRIPT_PREVIOUS_SCRIPT:
            path = NATRON_IMAGES_PATH "previousScript.png";
            break;
        case NATRON_PIXMAP_SCRIPT_SAVE_SCRIPT:
            path = NATRON_IMAGES_PATH "saveScript.png";
            break;
        case NATRON_PIXMAP_CURVE_EDITOR:
            path = NATRON_IMAGES_PATH "CurveEditorIcon.png";
            break;
        case NATRON_PIXMAP_PROGRESS_PANEL:
            path = NATRON_IMAGES_PATH "ProgressPanelIcon.png";
            break;
        case NATRON_PIXMAP_SCRIPT_EDITOR:
            path = NATRON_IMAGES_PATH "ScriptEditorIcon.png";
            break;
        case NATRON_PIXMAP_ANIMATION_MODULE:
            path = NATRON_IMAGES_PATH "DS_CE_Icon.png";
            break;
        case NATRON_PIXMAP_VIEWER_PANEL:
            path = NATRON_IMAGES_PATH "ViewerPanelIcon.png";
            break;
        case NATRON_PIXMAP_PROPERTIES_PANEL:
            path = NATRON_IMAGES_PATH "PropertiesIcon.png";
            break;
        case NATRON_PIXMAP_DOPE_SHEET:
            path = NATRON_IMAGES_PATH "DopeSheetIcon.png";
            break;
        case NATRON_PIXMAP_NODE_GRAPH:
            path = NATRON_IMAGES_PATH "NodeGraphIcon.png";
            break;

        case NATRON_PIXMAP_MERGE_ATOP:
            path = NATRON_IMAGES_PATH "merge_atop.png";
            break;
        case NATRON_PIXMAP_MERGE_AVERAGE:
            path = NATRON_IMAGES_PATH "merge_average.png";
            break;
        case NATRON_PIXMAP_MERGE_COLOR:
            path = NATRON_IMAGES_PATH "merge_color.png";
            break;
        case NATRON_PIXMAP_MERGE_COLOR_BURN:
            path = NATRON_IMAGES_PATH "merge_color_burn.png";
            break;
        case NATRON_PIXMAP_MERGE_COLOR_DODGE:
            path = NATRON_IMAGES_PATH "merge_color_dodge.png";
            break;
        case NATRON_PIXMAP_MERGE_CONJOINT_OVER:
            path = NATRON_IMAGES_PATH "merge_conjoint_over.png";
            break;
        case NATRON_PIXMAP_MERGE_COPY:
            path = NATRON_IMAGES_PATH "merge_copy.png";
            break;
        case NATRON_PIXMAP_MERGE_DIFFERENCE:
            path = NATRON_IMAGES_PATH "merge_difference.png";
            break;
        case NATRON_PIXMAP_MERGE_DISJOINT_OVER:
            path = NATRON_IMAGES_PATH "merge_disjoint_over.png";
            break;
        case NATRON_PIXMAP_MERGE_DIVIDE:
            path = NATRON_IMAGES_PATH "merge_divide.png";
            break;
        case NATRON_PIXMAP_MERGE_EXCLUSION:
            path = NATRON_IMAGES_PATH "merge_exclusion.png";
            break;
        case NATRON_PIXMAP_MERGE_FREEZE:
            path = NATRON_IMAGES_PATH "merge_freeze.png";
            break;
        case NATRON_PIXMAP_MERGE_FROM:
            path = NATRON_IMAGES_PATH "merge_from.png";
            break;
        case NATRON_PIXMAP_MERGE_GEOMETRIC:
            path = NATRON_IMAGES_PATH "merge_geometric.png";
            break;
        case NATRON_PIXMAP_MERGE_GRAIN_EXTRACT:
            path = NATRON_IMAGES_PATH "merge_grain_extract.png";
            break;
        case NATRON_PIXMAP_MERGE_GRAIN_MERGE:
            path = NATRON_IMAGES_PATH "merge_grain_merge.png";
            break;
        case NATRON_PIXMAP_MERGE_HARD_LIGHT:
            path = NATRON_IMAGES_PATH "merge_hard_light.png";
            break;
        case NATRON_PIXMAP_MERGE_HUE:
            path = NATRON_IMAGES_PATH "merge_hue.png";
            break;
        case NATRON_PIXMAP_MERGE_HYPOT:
            path = NATRON_IMAGES_PATH "merge_hypot.png";
            break;
        case NATRON_PIXMAP_MERGE_IN:
            path = NATRON_IMAGES_PATH "merge_in.png";
            break;
        case NATRON_PIXMAP_MERGE_LUMINOSITY:
            path = NATRON_IMAGES_PATH "merge_luminosity.png";
            break;
        case NATRON_PIXMAP_MERGE_MASK:
            path = NATRON_IMAGES_PATH "merge_mask.png";
            break;
        case NATRON_PIXMAP_MERGE_MATTE:
            path = NATRON_IMAGES_PATH "merge_matte.png";
            break;
        case NATRON_PIXMAP_MERGE_MAX:
            path = NATRON_IMAGES_PATH "merge_max.png";
            break;
        case NATRON_PIXMAP_MERGE_MIN:
            path = NATRON_IMAGES_PATH "merge_min.png";
            break;
        case NATRON_PIXMAP_MERGE_MINUS:
            path = NATRON_IMAGES_PATH "merge_minus.png";
            break;
        case NATRON_PIXMAP_MERGE_MULTIPLY:
            path = NATRON_IMAGES_PATH "merge_multiply.png";
            break;
        case NATRON_PIXMAP_MERGE_OUT:
            path = NATRON_IMAGES_PATH "merge_out.png";
            break;
        case NATRON_PIXMAP_MERGE_OVER:
            path = NATRON_IMAGES_PATH "merge_over.png";
            break;
        case NATRON_PIXMAP_MERGE_OVERLAY:
            path = NATRON_IMAGES_PATH "merge_overlay.png";
            break;
        case NATRON_PIXMAP_MERGE_PINLIGHT:
            path = NATRON_IMAGES_PATH "merge_pinlight.png";
            break;
        case NATRON_PIXMAP_MERGE_PLUS:
            path = NATRON_IMAGES_PATH "merge_plus.png";
            break;
        case NATRON_PIXMAP_MERGE_REFLECT:
            path = NATRON_IMAGES_PATH "merge_reflect.png";
            break;
        case NATRON_PIXMAP_MERGE_SATURATION:
            path = NATRON_IMAGES_PATH "merge_saturation.png";
            break;
        case NATRON_PIXMAP_MERGE_SCREEN:
            path = NATRON_IMAGES_PATH "merge_screen.png";
            break;
        case NATRON_PIXMAP_MERGE_SOFT_LIGHT:
            path = NATRON_IMAGES_PATH "merge_soft_light.png";
            break;
        case NATRON_PIXMAP_MERGE_STENCIL:
            path = NATRON_IMAGES_PATH "merge_stencil.png";
            break;
        case NATRON_PIXMAP_MERGE_UNDER:
            path = NATRON_IMAGES_PATH "merge_under.png";
            break;
        case NATRON_PIXMAP_MERGE_XOR:
            path = NATRON_IMAGES_PATH "merge_xor.png";
            break;


        case NATRON_PIXMAP_LINK_CURSOR:
            path = NATRON_IMAGES_PATH "linkCursor.png";
            break;
        case NATRON_PIXMAP_LINK_MULT_CURSOR:
            path = NATRON_IMAGES_PATH "linkMultCursor.png";
            break;
        case NATRON_PIXMAP_ENTER_GROUP:
            path = NATRON_IMAGES_PATH "enter_group.png";
            break;
        case NATRON_PIXMAP_ROTOPAINT_BUILDUP_ENABLED:
            path = NATRON_IMAGES_PATH "rotopaint_buildup_on.png";
            break;
        case NATRON_PIXMAP_ROTOPAINT_BUILDUP_DISABLED:
            path = NATRON_IMAGES_PATH "rotopaint_buildup_off.png";
            break;
        case NATRON_PIXMAP_ROTOPAINT_BLUR:
            path = NATRON_IMAGES_PATH "rotopaint_blur.png";
            break;
        case NATRON_PIXMAP_ROTOPAINT_BURN:
            path = NATRON_IMAGES_PATH "rotopaint_burn.png";
            break;
        case NATRON_PIXMAP_ROTOPAINT_CLONE:
            path = NATRON_IMAGES_PATH "rotopaint_clone.png";
            break;
        case NATRON_PIXMAP_ROTOPAINT_DODGE:
            path = NATRON_IMAGES_PATH "rotopaint_dodge.png";
            break;
        case NATRON_PIXMAP_ROTOPAINT_ERASER:
            path = NATRON_IMAGES_PATH "rotopaint_eraser.png";
            break;
        case NATRON_PIXMAP_ROTOPAINT_PRESSURE_ENABLED:
            path = NATRON_IMAGES_PATH "rotopaint_pressure_on.png";
            break;
        case NATRON_PIXMAP_ROTOPAINT_PRESSURE_DISABLED:
            path = NATRON_IMAGES_PATH "rotopaint_pressure_off.png";
            break;
        case NATRON_PIXMAP_ROTOPAINT_REVEAL:
            path = NATRON_IMAGES_PATH "rotopaint_reveal.png";
            break;
        case NATRON_PIXMAP_ROTOPAINT_SHARPEN:
            path = NATRON_IMAGES_PATH "rotopaint_sharpen.png";
            break;
        case NATRON_PIXMAP_ROTOPAINT_SMEAR:
            path = NATRON_IMAGES_PATH "rotopaint_smear.png";
            break;
        case NATRON_PIXMAP_ROTOPAINT_SOLID:
            path = NATRON_IMAGES_PATH "rotopaint_solid.png";
            break;
        case NATRON_PIXMAP_ROTO_NODE_ICON:
            path = NATRON_IMAGES_PATH "rotoNodeIcon.png";
            break;
        case NATRON_PIXMAP_INTERP_LINEAR:
            path = NATRON_IMAGES_PATH "interp_linear.png";
            break;
        case NATRON_PIXMAP_INTERP_CURVE:
            path = NATRON_IMAGES_PATH "interp_curve.png";
            break;
        case NATRON_PIXMAP_INTERP_CONSTANT:
            path = NATRON_IMAGES_PATH "interp_constant.png";
            break;
        case NATRON_PIXMAP_INTERP_BREAK:
            path = NATRON_IMAGES_PATH "interp_break.png";
            break;
        case NATRON_PIXMAP_INTERP_CURVE_C:
            path = NATRON_IMAGES_PATH "interp_curve_c.png";
            break;
        case NATRON_PIXMAP_INTERP_CURVE_H:
            path = NATRON_IMAGES_PATH "interp_curve_h.png";
            break;
        case NATRON_PIXMAP_INTERP_CURVE_R:
            path = NATRON_IMAGES_PATH "interp_curve_r.png";
            break;
        case NATRON_PIXMAP_INTERP_CURVE_Z:
            path = NATRON_IMAGES_PATH "interp_curve_z.png";
            break;
            // DON'T add a default: case here
        } // switch
        if ( path.empty() ) {
            assert(!"Missing image.");
        }
        // put a breakpoint in png_chunk_report to catch the error "libpng warning: iCCP: known incorrect sRGB profile"

        // old version:
        //QImage img;
        //img.load(path);
        //*pix = QPixmap::fromImage(img);

        // new version:
        pix->load( QString::fromUtf8( path.c_str() ) );

        QPixmapCache::insert(QString::number(e), *pix);
    }
} // getIcon

void
GuiApplicationManager::getIcon(PixmapEnum e,
                               int size,
                               QPixmap* pix) const
{
    if ( !QPixmapCache::find(QString::number(e) + QLatin1Char('@') + QString::number(size), pix) ) {
        getIcon(e, pix);
        if (std::max( pix->width(), pix->height() ) != size) {
            *pix = pix->scaled(size, size, Qt::KeepAspectRatio, Qt::SmoothTransformation);
            QPixmapCache::insert(QString::number(e), *pix);
        }
    }
}

const std::list<PluginGroupNodePtr>&
GuiApplicationManager::getTopLevelPluginsToolButtons() const
{
    return _imp->_topLevelToolButtons;
}

const QCursor &
GuiApplicationManager::getColorPickerCursor() const
{
    return _imp->_colorPickerCursor;
}

const QCursor &
GuiApplicationManager::getLinkToCursor() const
{
    return _imp->_linkToCursor;
}

const QCursor &
GuiApplicationManager::getLinkToMultCursor() const
{
    return _imp->_linkMultCursor;
}

bool
GuiApplicationManager::initGui(const CLArgs& args)
{

#ifdef __NATRON_UNIX__
#ifndef __NATRON_OSX__
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
    // workaround for issues with KDE4
    QApplication::setStyle( QString::fromUtf8("plastique") );
#endif
#endif
#endif

    //load custom fonts
    QString fontResource = QString::fromUtf8(":/Resources/Fonts/%1.ttf");
    QStringList fontFilenames;

    fontFilenames << fontResource.arg( QString::fromUtf8("DroidSans") );
    fontFilenames << fontResource.arg( QString::fromUtf8("DroidSans-Bold") );
    Q_FOREACH (const QString &fontFilename, fontFilenames) {
        int fontID = QFontDatabase::addApplicationFont(fontFilename);

        qDebug() << "fontID=" << fontID << "families=" << QFontDatabase::applicationFontFamilies(fontID);
    }
    QString fontFamily = QString::fromUtf8(getCurrentSettings()->getApplicationFontFamily().c_str());
    int fontSize = getCurrentSettings()->getApplicationFontSize();


    //fontFamily = "Courier"; fontSize = 24; // for debugging purposes
    qDebug() << "Setting application font to " << fontFamily << fontSize;
    {
        QFontDatabase database;
        Q_FOREACH ( const QString &family, database.families() ) {
            if (family == fontFamily) {
                qDebug() << "... found" << fontFamily << "available styles:";
                Q_FOREACH ( const QString &style, database.styles(family) ) {
                    qDebug() << family << style;
                }
            }
        }
        QFont font(fontFamily, fontSize);
        if ( !font.exactMatch() ) {
            QFontInfo fi(font);
            qDebug() << "Not an exact match, got: " << fi.family() << fi.pointSize();
        }
        QApplication::setFont(font);
#ifdef Q_OS_MAC
        // https://bugreports.qt.io/browse/QTBUG-32789
        QFont::insertSubstitution(QString::fromUtf8(".Lucida Grande UI"), fontFamily /*"Lucida Grande"*/);
        // https://bugreports.qt.io/browse/QTBUG-40833
        QFont::insertSubstitution(QString::fromUtf8(".Helvetica Neue DeskInterface"), fontFamily /*"Helvetica Neue"*/);
        // there are lots of remaining bugs on Yosemite in 4.8.6, to be fixed in 4.8.7&
#endif
    }
    _imp->_fontFamily = fontFamily;
    _imp->_fontSize = fontSize;

    /*Display a splashscreen while we wait for the engine to load*/
    QString filename = QString::fromUtf8(NATRON_IMAGES_PATH "splashscreen.jpg");

    _imp->_splashScreen = new SplashScreen(filename);
    _imp->_splashScreen->setAttribute(Qt::WA_DeleteOnClose, 0);

    {
#ifdef DEBUG
        boost_adaptbx::floating_point::exception_trapping trap(0);
#endif
        QCoreApplication::processEvents();
    }
    QPixmap appIcPixmap;
    appPTR->getIcon(NATRON_PIXMAP_APP_ICON, &appIcPixmap);
    QIcon appIc(appIcPixmap);
    qApp->setWindowIcon(appIc);

    QFontDatabase db;
    QStringList families = db.families(QFontDatabase::Latin); // We need a Latin font for the UI
    std::vector<std::string> systemFonts( families.size() );
    for (int i = 0; i < families.size(); ++i) {
        systemFonts[i] = families[i].toStdString();
    }
    getCurrentSettings()->populateSystemFonts(systemFonts);

    _imp->createColorPickerCursor();
    _imp->createLinkToCursor();
    _imp->createLinkMultCursor();

    ///Copy the args that will be used when fontconfig cache is updated
    _imp->startupArgs = args;


    _imp->fontconfigUpdateWatcher.reset(new QFutureWatcher<void>);
    QObject::connect( _imp->fontconfigUpdateWatcher.get(), SIGNAL(finished()), this, SLOT(onFontconfigCacheUpdateFinished()) );
    setLoadingStatus( tr("Updating fontconfig cache...") );

    QObject::connect( &_imp->updateSplashscreenTimer, SIGNAL(timeout()), this, SLOT(onFontconfigTimerTriggered()) );
    _imp->updateSplashscreenTimer.start(1000);

    _imp->fontconfigUpdateWatcher->setFuture( QtConcurrent::run(_imp.get(), &GuiApplicationManagerPrivate::updateFontConfigCache) );

    Gui::loadStyleSheet();

    // Init documentation manager
    _imp->documentation.reset(new DocumentationManager);
    _imp->documentation->startServer();

    return exec();
} // GuiApplicationManager::initGui

void
GuiApplicationManager::onFontconfigCacheUpdateFinished()
{
    _imp->updateSplashscreenTimer.stop();

    ///Ignore return value here, the user will have an error dialog if loading failed anyway.
    bool ret = loadInternalAfterInitGui(_imp->startupArgs);
    Q_UNUSED(ret);
}

void
GuiApplicationManager::onFontconfigTimerTriggered()
{
    _imp->fontconfigMessageDots = (_imp->fontconfigMessageDots + 1) % 4;

    QString message = tr("Updating fontconfig cache");
    for (int i = 0; i < _imp->fontconfigMessageDots; ++i) {
        message.append( QLatin1Char('.') );
    }
    setLoadingStatus(message);
}

void
GuiApplicationManager::onPluginLoaded(const PluginPtr& plugin)
{
    AppManager::onPluginLoaded(plugin);
    
    // Ensure the toolbutton is created for this plug-in with its grouping hierarchy
    PluginGroupNodePtr child = findPluginToolButtonOrCreate(plugin);
    Q_UNUSED(child);

} // GuiApplicationManager::onPluginLoaded



PluginGroupNodePtr
GuiApplicationManager::findPluginToolButtonOrCreate(const PluginPtr& plugin)
{
    std::vector<std::string> grouping = plugin->getPropertyNUnsafe<std::string>(kNatronPluginPropGrouping);
    std::vector<std::string> iconGrouping = plugin->getPropertyNUnsafe<std::string>(kNatronPluginPropGroupIconFilePath);
    QStringList qGroup, qIconGrouping;
    for (std::size_t i = 0; i < grouping.size(); ++i) {
        qGroup.push_back(QString::fromUtf8(grouping[i].c_str()));
    }
    for (std::size_t i = 0; i < iconGrouping.size(); ++i) {
        qIconGrouping.push_back(QString::fromUtf8(iconGrouping[i].c_str()));
    }
    return _imp->findPluginToolButtonOrCreateInternal(_imp->_topLevelToolButtons,
                                                      PluginGroupNodePtr(),
                                                      plugin,
                                                      qGroup,
                                                      qIconGrouping);
}

bool
GuiApplicationManager::isSplashcreenVisible() const
{
    return _imp->_splashScreen ? true : false;
}

void
GuiApplicationManager::hideSplashScreen()
{
#ifdef DEBUG
    boost_adaptbx::floating_point::exception_trapping trap(0);
#endif
    if (_imp->_splashScreen) {
        _imp->_splashScreen->close();
        delete _imp->_splashScreen;
        _imp->_splashScreen = 0;
    }
}

void
GuiApplicationManager::setKnobClipBoard(KnobClipBoardType type,
                                        const KnobIPtr& serialization,
                                        DimSpec dimension,
                                        ViewSetSpec view)
{
    _imp->_knobsClipBoard->serialization = serialization;
    _imp->_knobsClipBoard->type = type;
    _imp->_knobsClipBoard->dimension = dimension;
    _imp->_knobsClipBoard->view = view;
}

void
GuiApplicationManager::getKnobClipBoard(KnobClipBoardType *type,
                                        KnobIPtr *serialization,
                                        DimSpec* dimension,
                                        ViewSetSpec *view) const
{
    *serialization = _imp->_knobsClipBoard->serialization;
    *type = _imp->_knobsClipBoard->type;
    *dimension = _imp->_knobsClipBoard->dimension;
    *view = _imp->_knobsClipBoard->view;
}

void
GuiApplicationManager::appendTaskToPreviewThread(const NodeGuiPtr& node,
                                                 TimeValue time)
{
    _imp->previewRenderThread.appendToQueue(node, time);
}

int
GuiApplicationManager::getDocumentationServerPort()
{
    return _imp->documentation->serverPort();
}

double
GuiApplicationManager::getLogicalDPIXRATIO() const
{
    return _imp->dpiX / NATRON_PIXELS_FOR_DPI_DEFAULT;
}

double
GuiApplicationManager::getLogicalDPIYRATIO() const
{
    return _imp->dpiY / NATRON_PIXELS_FOR_DPI_DEFAULT;
}

void
GuiApplicationManager::setCurrentLogicalDPI(double dpiX,
                                            double dpiY)
{
    _imp->dpiX = dpiX;
    _imp->dpiY = dpiY;
}


void
GuiApplicationManager::updateAboutWindowLibrariesVersion()
{
    const AppInstanceVec& instances = getAppInstances();

    for (AppInstanceVec::const_iterator it = instances.begin(); it != instances.end(); ++it) {
        GuiAppInstancePtr isGuiInstance = toGuiAppInstance(*it);
        if (isGuiInstance) {
            Gui* gui = isGuiInstance->getGui();
            if (gui) {
                gui->updateAboutWindowLibrariesVersion();
            }
        }
    }
}

NATRON_NAMESPACE_EXIT

NATRON_NAMESPACE_USING
#include "moc_GuiApplicationManager.cpp"
