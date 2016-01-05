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

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "GuiApplicationManager.h"
#include "GuiApplicationManagerPrivate.h"

///gui
#include "Global/Macros.h"

CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QtCore/QDebug>
#include <QtCore/QSettings>
#include <QPixmapCache>
#include <QApplication>
#include <QFontDatabase>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)


#include <QtConcurrentRun>

#include "Engine/Settings.h"
#include "Engine/EffectInstance.h" // PLUGINID_OFX_*

#include "Gui/GuiDefines.h"
#include "Gui/KnobGuiFactory.h"
#include "Gui/SplashScreen.h"


using namespace Natron;


GuiApplicationManager::GuiApplicationManager()
: AppManager()
, _imp(new GuiApplicationManagerPrivate(this))
{
}

GuiApplicationManager::~GuiApplicationManager()
{
    delete _imp->_colorPickerCursor;
    for (AppShortcuts::iterator it = _imp->_actionShortcuts.begin(); it != _imp->_actionShortcuts.end(); ++it) {
        for (GroupShortcuts::iterator it2 = it->second.begin(); it2 != it->second.end(); ++it2) {
            delete it2->second;
        }
    }
}

void
GuiApplicationManager::getIcon(Natron::PixmapEnum e,
                               QPixmap* pix) const
{
    int iconSet = appPTR->getCurrentSettings()->getIconsBlackAndWhite() ? 2 : 3;
    QString iconSetStr = QString::number(iconSet);

    if ( !QPixmapCache::find(QString::number(e),pix) ) {
        QString path;
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
            case NATRON_PIXMAP_PLAYER_STOP:
                path = NATRON_IMAGES_PATH "stop.png";
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
        if (path.isEmpty()) {
            assert(!"Missing image.");
        }
        // put a breakpoint in png_chunk_report to catch the error "libpng warning: iCCP: known incorrect sRGB profile"

        // old version:
        //QImage img;
        //img.load(path);
        //*pix = QPixmap::fromImage(img);

        // new version:
        pix->load(path);

        QPixmapCache::insert(QString::number(e), *pix);
    }
} // getIcon

void
GuiApplicationManager::getIcon(Natron::PixmapEnum e,
                               int size,
                               QPixmap* pix) const
{
    if ( !QPixmapCache::find(QString::number(e) + '@' + QString::number(size), pix) ) {
        getIcon(e, pix);
        if (std::max(pix->width(), pix->height()) != size) {
            *pix = pix->scaled(size, size, Qt::KeepAspectRatio, Qt::SmoothTransformation);
            QPixmapCache::insert(QString::number(e), *pix);
        }
    }
}

const std::list<boost::shared_ptr<PluginGroupNode> >&
GuiApplicationManager::getTopLevelPluginsToolButtons() const
{
    return _imp->_topLevelToolButtons;
}

const QCursor &
GuiApplicationManager::getColorPickerCursor() const
{
    return *(_imp->_colorPickerCursor);
}

bool
GuiApplicationManager::initGui(const CLArgs& args)
{
    QSettings settings(NATRON_ORGANIZATION_NAME,NATRON_APPLICATION_NAME);

    //load custom fonts
    QString fontResource = QString(":/Resources/Fonts/%1.ttf");
    QStringList fontFilenames;
    fontFilenames << fontResource.arg("DroidSans");
    fontFilenames << fontResource.arg("DroidSans-Bold");
    Q_FOREACH (QString fontFilename, fontFilenames) {
        int fontID = QFontDatabase::addApplicationFont(fontFilename);
        qDebug() << "fontID=" << fontID << "families=" << QFontDatabase::applicationFontFamilies(fontID);
    }
    QString fontFamily(NATRON_FONT);
    int fontSize = NATRON_FONT_SIZE_11;
    
    
    ///Do not load old font stored in the setting "systemFont" on Natron < 2 because it might contain a crappy font.
    if (settings.contains(kQSettingsSoftwareMajorVersionSettingName) && settings.value(kQSettingsSoftwareMajorVersionSettingName).toInt() >= 2) {
        if (settings.contains("systemFont")) {
            fontFamily = settings.value("systemFont").toString();
        }
    }
    
    
    if (settings.contains("fontSize")) {
        fontSize = settings.value("fontSize").toInt();
    }
    //fontFamily = "Courier"; fontSize = 24; // for debugging purposes
    qDebug() << "Setting application font to " << fontFamily << fontSize;
    {
        QFontDatabase database;
        Q_FOREACH (const QString &family, database.families()) {
            if (family == fontFamily) {
                qDebug() << "... found" << fontFamily << "available styles:";
                Q_FOREACH (const QString &style, database.styles(family)) {
                    qDebug() << family << style;
                }
            }
        }
        QFont font(fontFamily, fontSize);
        if (!font.exactMatch()) {
            QFontInfo fi(font);
            qDebug() << "Not an exact match, got: " << fi.family() << fi.pointSize();
        }
        QApplication::setFont(font);
#ifdef Q_OS_MAC
        // https://bugreports.qt.io/browse/QTBUG-32789
        QFont::insertSubstitution(".Lucida Grande UI", fontFamily/*"Lucida Grande"*/);
        // https://bugreports.qt.io/browse/QTBUG-40833
        QFont::insertSubstitution(".Helvetica Neue DeskInterface", fontFamily/*"Helvetica Neue"*/);
        // there are lots of remaining bugs on Yosemite in 4.8.6, to be fixed in 4.8.7&
#endif
    }
    _imp->_fontFamily = fontFamily;
    _imp->_fontSize = fontSize;

    /*Display a splashscreen while we wait for the engine to load*/
    QString filename(NATRON_IMAGES_PATH "splashscreen.png");

    _imp->_splashScreen = new SplashScreen(filename);
    _imp->_splashScreen->setAttribute(Qt::WA_DeleteOnClose, 0);
    
    QCoreApplication::processEvents();
    QPixmap appIcPixmap;
    appPTR->getIcon(Natron::NATRON_PIXMAP_APP_ICON, &appIcPixmap);
    QIcon appIc(appIcPixmap);
    qApp->setWindowIcon(appIc);
    
    QFontDatabase db;
    QStringList families = db.families(QFontDatabase::Latin); // We need a Latin font for the UI
    std::vector<std::string> systemFonts(families.size());
    for (int i = 0; i < families.size(); ++i) {
        systemFonts[i] = families[i].toStdString();
    }
    getCurrentSettings()->populateSystemFonts(settings,systemFonts);
    
    _imp->createColorPickerCursor();
    _imp->_knobsClipBoard->isEmpty = true;
    
    ///Copy the args that will be used when fontconfig cache is updated
    _imp->startupArgs = args;
    
    
    _imp->fontconfigUpdateWatcher.reset(new QFutureWatcher<void>);
    QObject::connect(_imp->fontconfigUpdateWatcher.get(), SIGNAL(finished()), this, SLOT(onFontconfigCacheUpdateFinished()));
    setLoadingStatus( QObject::tr("Updating fontconfig cache...") );
    
    QObject::connect(&_imp->updateSplashscreenTimer, SIGNAL(timeout()), this, SLOT(onFontconfigTimerTriggered()));
    _imp->updateSplashscreenTimer.start(1000);
    
    _imp->fontconfigUpdateWatcher->setFuture(QtConcurrent::run(_imp.get(),&GuiApplicationManagerPrivate::updateFontConfigCache));
    
    
    
    return exec();
}

void
GuiApplicationManager::onFontconfigCacheUpdateFinished()
{

    _imp->updateSplashscreenTimer.stop();
    
    ///Ignore return value here, the user will have an error dialog if loading failed anyway.
    (void)loadInternalAfterInitGui(_imp->startupArgs);
}

void
GuiApplicationManager::onFontconfigTimerTriggered()
{
    _imp->fontconfigMessageDots = (_imp->fontconfigMessageDots + 1) % 4;
    
    QString message = tr("Updating fontconfig cache");
    for (int i = 0; i < _imp->fontconfigMessageDots; ++i) {
        message.append('.');
    }
    setLoadingStatus(message);
}

void
GuiApplicationManager::onPluginLoaded(Natron::Plugin* plugin)
{
    QString shortcutGrouping(kShortcutGroupNodes);
    const QStringList & groups = plugin->getGrouping();
    const QString & pluginID = plugin->getPluginID();
    const QString  pluginLabel = plugin->getLabelWithoutSuffix();
    const QString & pluginIconPath = plugin->getIconFilePath();
    const QStringList & groupIconPath = plugin->getGroupIconFilePath();

    QStringList groupingWithID = groups;
    groupingWithID.push_back(pluginID);
    boost::shared_ptr<PluginGroupNode> child = findPluginToolButtonOrCreate(groupingWithID,
                                                                            pluginLabel,
                                                                            groupIconPath,
                                                                            pluginIconPath,
                                                                            plugin->getMajorVersion(),
                                                                            plugin->getMinorVersion(),
                                                                            plugin->getIsUserCreatable());
    for (int i = 0; i < groups.size(); ++i) {

        shortcutGrouping.push_back('/');
        shortcutGrouping.push_back(groups[i]);
    }
    
    Qt::KeyboardModifiers modifiers = Qt::NoModifier;
    Qt::Key symbol = (Qt::Key)0;
    bool hasShortcut = true;

    /*These are the plug-ins which have a default shortcut. Other plug-ins can have a user-assigned shortcut.*/
    if (pluginID == PLUGINID_OFX_TRANSFORM) {
        symbol = Qt::Key_T;
    } else if (pluginID == PLUGINID_NATRON_ROTO) {
        symbol = Qt::Key_O;
    } else if (pluginID == PLUGINID_NATRON_ROTOPAINT) {
        symbol = Qt::Key_P;
    } else if (pluginID == PLUGINID_OFX_MERGE) {
        symbol = Qt::Key_M;
    } else if (pluginID == PLUGINID_OFX_GRADE) {
        symbol = Qt::Key_G;
    } else if (pluginID == PLUGINID_OFX_COLORCORRECT) {
        symbol = Qt::Key_C;
    } else if (pluginID == PLUGINID_OFX_BLURCIMG) {
        symbol = Qt::Key_B;
    } else if (pluginID == PLUGINID_NATRON_DOT) {
        symbol = Qt::Key_Period;
        modifiers |= Qt::ShiftModifier;
    } else {
        hasShortcut = false;
    }
    plugin->setHasShortcut(hasShortcut);
    
    if (plugin->getIsUserCreatable()) {
        _imp->addKeybind(shortcutGrouping, pluginID, pluginLabel, modifiers, symbol);
    }
    
}

void
GuiApplicationManager::ignorePlugin(Natron::Plugin* plugin)
{
    _imp->removePluginToolButton(plugin->getGrouping());
    _imp->removeKeybind(kShortcutGroupNodes, plugin->getPluginID());
}


boost::shared_ptr<PluginGroupNode>
GuiApplicationManager::findPluginToolButtonOrCreate(const QStringList & grouping,
                                                    const QString & name,
                                                    const QStringList& groupIconPath,
                                                    const QString & iconPath,
                                                    int major,
                                                    int minor,
                                                    bool isUserCreatable)
{
    assert(grouping.size() > 0);
    return _imp->findPluginToolButtonInternal(_imp->_topLevelToolButtons, boost::shared_ptr<PluginGroupNode>(), grouping, name, groupIconPath, iconPath, major, minor, isUserCreatable);

}

bool
GuiApplicationManager::isSplashcreenVisible() const
{
    return _imp->_splashScreen ? true : false;
}

void
GuiApplicationManager::hideSplashScreen()
{
    if (_imp->_splashScreen) {
        _imp->_splashScreen->close();
        delete _imp->_splashScreen;
        _imp->_splashScreen = 0;
    }
}

bool
GuiApplicationManager::isClipBoardEmpty() const
{
    return _imp->_knobsClipBoard->isEmpty;
}

void
GuiApplicationManager::setKnobClipBoard(bool copyAnimation,
                                        const std::list<Variant> & values,
                                        const std::list<boost::shared_ptr<Curve> > & animation,
                                        const std::map<int,std::string> & stringAnimation,
                                        const std::list<boost::shared_ptr<Curve> > & parametricCurves,
                                        const std::string& appID,
                                        const std::string& nodeFullyQualifiedName,
                                        const std::string& paramName)
{
    _imp->_knobsClipBoard->copyAnimation = copyAnimation;
    _imp->_knobsClipBoard->isEmpty = false;
    _imp->_knobsClipBoard->values = values;
    _imp->_knobsClipBoard->curves = animation;
    _imp->_knobsClipBoard->stringAnimation = stringAnimation;
    _imp->_knobsClipBoard->parametricCurves = parametricCurves;
    _imp->_knobsClipBoard->appID = appID;
    _imp->_knobsClipBoard->nodeFullyQualifiedName = nodeFullyQualifiedName;
    _imp->_knobsClipBoard->paramName = paramName;
}

void
GuiApplicationManager::getKnobClipBoard(bool* copyAnimation,
                                        std::list<Variant>* values,
                                        std::list<boost::shared_ptr<Curve> >* animation,
                                        std::map<int,std::string>* stringAnimation,
                                        std::list<boost::shared_ptr<Curve> >* parametricCurves,
                                        std::string* appID,
                                        std::string* nodeFullyQualifiedName,
                                        std::string* paramName) const
{
    *copyAnimation = _imp->_knobsClipBoard->copyAnimation;
    *values = _imp->_knobsClipBoard->values;
    *animation = _imp->_knobsClipBoard->curves;
    *stringAnimation = _imp->_knobsClipBoard->stringAnimation;
    *parametricCurves = _imp->_knobsClipBoard->parametricCurves;
    *appID = _imp->_knobsClipBoard->appID;
    *nodeFullyQualifiedName = _imp->_knobsClipBoard->nodeFullyQualifiedName;
    *paramName = _imp->_knobsClipBoard->paramName;
}
