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

#ifndef NATRON_GLOBAL_ENUMS_H
#define NATRON_GLOBAL_ENUMS_H

#include "Global/Macros.h"

#include <QFlags>
CLANG_DIAG_OFF(deprecated)
#include <QMetaType>
CLANG_DIAG_ON(deprecated)

namespace Natron {
enum ScaleTypeEnum
{
    eScaleTypeLinear,
    eScaleTypeLog,
};

enum TimelineStateEnum
{
    eTimelineStateIdle,
    eTimelineStateDraggingCursor,
    eTimelineStateDraggingBoundary,
    eTimelineStatePanning,
    eTimelineStateSelectingZoomRange,
};

enum TimelineChangeReasonEnum
{
    eTimelineChangeReasonUserSeek = 0,
    eTimelineChangeReasonPlaybackSeek = 1,
    eTimelineChangeReasonCurveEditorSeek = 2,
    eTimelineChangeReasonDopeSheetEditorSeek = 3,
    eTimelineChangeReasonOtherSeek
};

enum StatusEnum
{
    eStatusOK = 0,
    eStatusFailed = 1,
    eStatusReplyDefault = 14
};

/*Copy of QMessageBox::StandardButton*/
enum StandardButtonEnum
{
    eStandardButtonNoButton           = 0x00000000,
    eStandardButtonEscape             = 0x00000200,            // obsolete
    eStandardButtonOk                 = 0x00000400,
    eStandardButtonSave               = 0x00000800,
    eStandardButtonSaveAll            = 0x00001000,
    eStandardButtonOpen               = 0x00002000,
    eStandardButtonYes                = 0x00004000,
    eStandardButtonYesToAll           = 0x00008000,
    eStandardButtonNo                 = 0x00010000,
    eStandardButtonNoToAll            = 0x00020000,
    eStandardButtonAbort              = 0x00040000,
    eStandardButtonRetry              = 0x00080000,
    eStandardButtonIgnore             = 0x00100000,
    eStandardButtonClose              = 0x00200000,
    eStandardButtonCancel             = 0x00400000,
    eStandardButtonDiscard            = 0x00800000,
    eStandardButtonHelp               = 0x01000000,
    eStandardButtonApply              = 0x02000000,
    eStandardButtonReset              = 0x04000000,
    eStandardButtonRestoreDefaults    = 0x08000000
};

typedef QFlags<Natron::StandardButtonEnum> StandardButtons;

enum MessageTypeEnum
{
    eMessageTypeInfo = 0,
    eMessageTypeError = 1,
    eMessageTypeWarning = 2,
    eMessageTypeQuestion = 3
};

enum KeyframeTypeEnum
{
    eKeyframeTypeConstant = 0,
    eKeyframeTypeLinear = 1,
    eKeyframeTypeSmooth = 2,
    eKeyframeTypeCatmullRom = 3,
    eKeyframeTypeCubic = 4,
    eKeyframeTypeHorizontal = 5,
    eKeyframeTypeFree = 6,
    eKeyframeTypeBroken = 7,
    eKeyframeTypeNone = 8
};

enum PixmapEnum
{
    NATRON_PIXMAP_PLAYER_PREVIOUS = 0,
    NATRON_PIXMAP_PLAYER_FIRST_FRAME,
    NATRON_PIXMAP_PLAYER_NEXT,
    NATRON_PIXMAP_PLAYER_LAST_FRAME,
    NATRON_PIXMAP_PLAYER_NEXT_INCR,
    NATRON_PIXMAP_PLAYER_NEXT_KEY,
    NATRON_PIXMAP_PLAYER_PREVIOUS_INCR,
    NATRON_PIXMAP_PLAYER_PREVIOUS_KEY,
    NATRON_PIXMAP_PLAYER_REWIND_ENABLED,
    NATRON_PIXMAP_PLAYER_REWIND_DISABLED,
    NATRON_PIXMAP_PLAYER_PLAY_ENABLED,
    NATRON_PIXMAP_PLAYER_PLAY_DISABLED,
    NATRON_PIXMAP_PLAYER_STOP,
    NATRON_PIXMAP_PLAYER_LOOP_MODE,
    NATRON_PIXMAP_PLAYER_BOUNCE,
    NATRON_PIXMAP_PLAYER_PLAY_ONCE,


    NATRON_PIXMAP_MAXIMIZE_WIDGET,
    NATRON_PIXMAP_MINIMIZE_WIDGET,
    NATRON_PIXMAP_CLOSE_WIDGET,
    NATRON_PIXMAP_HELP_WIDGET,
    NATRON_PIXMAP_CLOSE_PANEL,
    NATRON_PIXMAP_HIDE_UNMODIFIED,
    NATRON_PIXMAP_UNHIDE_UNMODIFIED,
    
    NATRON_PIXMAP_GROUPBOX_FOLDED,
    NATRON_PIXMAP_GROUPBOX_UNFOLDED,

    NATRON_PIXMAP_UNDO,
    NATRON_PIXMAP_REDO,
    NATRON_PIXMAP_UNDO_GRAYSCALE,
    NATRON_PIXMAP_REDO_GRAYSCALE,
    NATRON_PIXMAP_RESTORE_DEFAULTS_ENABLED,
    NATRON_PIXMAP_RESTORE_DEFAULTS_DISABLED,

    NATRON_PIXMAP_TAB_WIDGET_LAYOUT_BUTTON,
    NATRON_PIXMAP_TAB_WIDGET_LAYOUT_BUTTON_ANCHOR,
    NATRON_PIXMAP_TAB_WIDGET_SPLIT_HORIZONTALLY,
    NATRON_PIXMAP_TAB_WIDGET_SPLIT_VERTICALLY,

    NATRON_PIXMAP_VIEWER_CENTER,
    NATRON_PIXMAP_VIEWER_CLIP_TO_PROJECT_ENABLED,
    NATRON_PIXMAP_VIEWER_CLIP_TO_PROJECT_DISABLED,
    NATRON_PIXMAP_VIEWER_REFRESH,
    NATRON_PIXMAP_VIEWER_REFRESH_ACTIVE,
    NATRON_PIXMAP_VIEWER_ROI_ENABLED,
    NATRON_PIXMAP_VIEWER_ROI_DISABLED,
    NATRON_PIXMAP_VIEWER_RENDER_SCALE,
    NATRON_PIXMAP_VIEWER_RENDER_SCALE_CHECKED,
    NATRON_PIXMAP_VIEWER_AUTOCONTRAST_ENABLED,
    NATRON_PIXMAP_VIEWER_AUTOCONTRAST_DISABLED,

    NATRON_PIXMAP_OPEN_FILE,
    NATRON_PIXMAP_COLORWHEEL,
    NATRON_PIXMAP_OVERLAY,
    NATRON_PIXMAP_ROTO_MERGE,
    NATRON_PIXMAP_COLOR_PICKER,

    NATRON_PIXMAP_IO_GROUPING,
    NATRON_PIXMAP_3D_GROUPING,
    NATRON_PIXMAP_CHANNEL_GROUPING,
    NATRON_PIXMAP_MERGE_GROUPING,
    NATRON_PIXMAP_COLOR_GROUPING,
    NATRON_PIXMAP_TRANSFORM_GROUPING,
    NATRON_PIXMAP_DEEP_GROUPING,
    NATRON_PIXMAP_FILTER_GROUPING,
    NATRON_PIXMAP_MULTIVIEW_GROUPING,
    NATRON_PIXMAP_TOOLSETS_GROUPING,
    NATRON_PIXMAP_MISC_GROUPING,
    NATRON_PIXMAP_OPEN_EFFECTS_GROUPING,
    NATRON_PIXMAP_TIME_GROUPING,
    NATRON_PIXMAP_PAINT_GROUPING,
    NATRON_PIXMAP_KEYER_GROUPING,
    NATRON_PIXMAP_OTHER_PLUGINS,

    NATRON_PIXMAP_READ_IMAGE,
    NATRON_PIXMAP_WRITE_IMAGE,

    NATRON_PIXMAP_COMBOBOX,
    NATRON_PIXMAP_COMBOBOX_PRESSED,

    NATRON_PIXMAP_ADD_KEYFRAME,
    NATRON_PIXMAP_REMOVE_KEYFRAME,

    NATRON_PIXMAP_INVERTED,
    NATRON_PIXMAP_UNINVERTED,
    NATRON_PIXMAP_VISIBLE,
    NATRON_PIXMAP_UNVISIBLE,
    NATRON_PIXMAP_LOCKED,
    NATRON_PIXMAP_UNLOCKED,
    NATRON_PIXMAP_LAYER,
    NATRON_PIXMAP_BEZIER,
    NATRON_PIXMAP_PENCIL,
    NATRON_PIXMAP_CURVE,
    NATRON_PIXMAP_BEZIER_32,
    NATRON_PIXMAP_ELLIPSE,
    NATRON_PIXMAP_RECTANGLE,
    NATRON_PIXMAP_ADD_POINTS,
    NATRON_PIXMAP_REMOVE_POINTS,
    NATRON_PIXMAP_CUSP_POINTS,
    NATRON_PIXMAP_SMOOTH_POINTS,
    NATRON_PIXMAP_REMOVE_FEATHER,
    NATRON_PIXMAP_OPEN_CLOSE_CURVE,
    NATRON_PIXMAP_SELECT_ALL,
    NATRON_PIXMAP_SELECT_POINTS,
    NATRON_PIXMAP_SELECT_FEATHER,
    NATRON_PIXMAP_SELECT_CURVES,
    NATRON_PIXMAP_AUTO_KEYING_ENABLED,
    NATRON_PIXMAP_AUTO_KEYING_DISABLED,
    NATRON_PIXMAP_STICKY_SELECTION_ENABLED,
    NATRON_PIXMAP_STICKY_SELECTION_DISABLED,
    NATRON_PIXMAP_FEATHER_LINK_ENABLED,
    NATRON_PIXMAP_FEATHER_LINK_DISABLED,
    NATRON_PIXMAP_FEATHER_VISIBLE,
    NATRON_PIXMAP_FEATHER_UNVISIBLE,
    NATRON_PIXMAP_RIPPLE_EDIT_ENABLED,
    NATRON_PIXMAP_RIPPLE_EDIT_DISABLED,
    NATRON_PIXMAP_ROTOPAINT_BLUR,
    NATRON_PIXMAP_ROTOPAINT_BUILDUP_ENABLED,
    NATRON_PIXMAP_ROTOPAINT_BUILDUP_DISABLED,
    NATRON_PIXMAP_ROTOPAINT_BURN,
    NATRON_PIXMAP_ROTOPAINT_CLONE,
    NATRON_PIXMAP_ROTOPAINT_DODGE,
    NATRON_PIXMAP_ROTOPAINT_ERASER,
    NATRON_PIXMAP_ROTOPAINT_PRESSURE_ENABLED,
    NATRON_PIXMAP_ROTOPAINT_PRESSURE_DISABLED,
    NATRON_PIXMAP_ROTOPAINT_REVEAL,
    NATRON_PIXMAP_ROTOPAINT_SHARPEN,
    NATRON_PIXMAP_ROTOPAINT_SMEAR,
    NATRON_PIXMAP_ROTOPAINT_SOLID,
    
    NATRON_PIXMAP_BOLD_CHECKED,
    NATRON_PIXMAP_BOLD_UNCHECKED,
    NATRON_PIXMAP_ITALIC_CHECKED,
    NATRON_PIXMAP_ITALIC_UNCHECKED,

    NATRON_PIXMAP_CLEAR_ALL_ANIMATION,
    NATRON_PIXMAP_CLEAR_BACKWARD_ANIMATION,
    NATRON_PIXMAP_CLEAR_FORWARD_ANIMATION,
    NATRON_PIXMAP_UPDATE_VIEWER_ENABLED,
    NATRON_PIXMAP_UPDATE_VIEWER_DISABLED,
    NATRON_PIXMAP_ADD_TRACK,
    
    NATRON_PIXMAP_ENTER_GROUP,
    
    NATRON_PIXMAP_SETTINGS,
    NATRON_PIXMAP_FREEZE_ENABLED,
    NATRON_PIXMAP_FREEZE_DISABLED,

    NATRON_PIXMAP_VIEWER_ICON,
    NATRON_PIXMAP_VIEWER_CHECKERBOARD_ENABLED,
    NATRON_PIXMAP_VIEWER_CHECKERBOARD_DISABLED,
    NATRON_PIXMAP_VIEWER_ZEBRA_ENABLED,
    NATRON_PIXMAP_VIEWER_ZEBRA_DISABLED,
    NATRON_PIXMAP_VIEWER_GAMMA_ENABLED,
    NATRON_PIXMAP_VIEWER_GAMMA_DISABLED,
    NATRON_PIXMAP_VIEWER_GAIN_ENABLED,
    NATRON_PIXMAP_VIEWER_GAIN_DISABLED,
    
    NATRON_PIXMAP_SCRIPT_CLEAR_OUTPUT,
    NATRON_PIXMAP_SCRIPT_EXEC_SCRIPT,
    NATRON_PIXMAP_SCRIPT_LOAD_EXEC_SCRIPT,
    NATRON_PIXMAP_SCRIPT_LOAD_SCRIPT,
    NATRON_PIXMAP_SCRIPT_NEXT_SCRIPT,
    NATRON_PIXMAP_SCRIPT_OUTPUT_PANE_ACTIVATED,
    NATRON_PIXMAP_SCRIPT_OUTPUT_PANE_DEACTIVATED,
    NATRON_PIXMAP_SCRIPT_PREVIOUS_SCRIPT,
    NATRON_PIXMAP_SCRIPT_SAVE_SCRIPT,
    
    NATRON_PIXMAP_MERGE_ATOP,
    NATRON_PIXMAP_MERGE_AVERAGE,
    NATRON_PIXMAP_MERGE_COLOR,
    NATRON_PIXMAP_MERGE_COLOR_BURN,
    NATRON_PIXMAP_MERGE_COLOR_DODGE,
    NATRON_PIXMAP_MERGE_CONJOINT_OVER,
    NATRON_PIXMAP_MERGE_COPY,
    NATRON_PIXMAP_MERGE_DIFFERENCE,
    NATRON_PIXMAP_MERGE_DISJOINT_OVER,
    NATRON_PIXMAP_MERGE_DIVIDE,
    NATRON_PIXMAP_MERGE_EXCLUSION,
    NATRON_PIXMAP_MERGE_FREEZE,
    NATRON_PIXMAP_MERGE_FROM,
    NATRON_PIXMAP_MERGE_GEOMETRIC,
    NATRON_PIXMAP_MERGE_GRAIN_EXTRACT,
    NATRON_PIXMAP_MERGE_GRAIN_MERGE,
    NATRON_PIXMAP_MERGE_HARD_LIGHT,
    NATRON_PIXMAP_MERGE_HUE,
    NATRON_PIXMAP_MERGE_HYPOT,
    NATRON_PIXMAP_MERGE_IN,
    NATRON_PIXMAP_MERGE_LUMINOSITY,
    NATRON_PIXMAP_MERGE_MASK,
    NATRON_PIXMAP_MERGE_MATTE,
    NATRON_PIXMAP_MERGE_MAX,
    NATRON_PIXMAP_MERGE_MIN,
    NATRON_PIXMAP_MERGE_MINUS,
    NATRON_PIXMAP_MERGE_MULTIPLY,
    NATRON_PIXMAP_MERGE_OUT,
    NATRON_PIXMAP_MERGE_OVER,
    NATRON_PIXMAP_MERGE_OVERLAY,
    NATRON_PIXMAP_MERGE_PINLIGHT,
    NATRON_PIXMAP_MERGE_PLUS,
    NATRON_PIXMAP_MERGE_REFLECT,
    NATRON_PIXMAP_MERGE_SATURATION,
    NATRON_PIXMAP_MERGE_SCREEN,
    NATRON_PIXMAP_MERGE_SOFT_LIGHT,
    NATRON_PIXMAP_MERGE_STENCIL,
    NATRON_PIXMAP_MERGE_UNDER,
    NATRON_PIXMAP_MERGE_XOR,
    
    NATRON_PIXMAP_ROTO_NODE_ICON,
    
    NATRON_PIXMAP_LINK_CURSOR,
    
    NATRON_PIXMAP_APP_ICON,

    NATRON_PIXMAP_INTERP_LINEAR,
    NATRON_PIXMAP_INTERP_CURVE,
    NATRON_PIXMAP_INTERP_CONSTANT,
    NATRON_PIXMAP_INTERP_BREAK,
    NATRON_PIXMAP_INTERP_CURVE_C,
    NATRON_PIXMAP_INTERP_CURVE_H,
    NATRON_PIXMAP_INTERP_CURVE_R,
    NATRON_PIXMAP_INTERP_CURVE_Z,
};


    
///This enum is used when dealing with parameters which have their value edited
enum ValueChangedReasonEnum
{
    //A user change to the knob triggered the call, gui will not be refreshed but instanceChangedAction called
    eValueChangedReasonUserEdited = 0,
    
    //A plugin change triggered the call, gui will be refreshed and instanceChangedAction called. This is stricly reserved
    //to calls made directly from an OpenFX plug-in
    eValueChangedReasonPluginEdited ,
    
    // Natron gui called setValue itself, instanceChangedAction will be called (with a reason of User edited) AND knob gui refreshed
    eValueChangedReasonNatronGuiEdited,
    
    // Natron engine called setValue itself, instanceChangedAction will be called (with a reason of plugin edited) AND knob gui refreshed
    eValueChangedReasonNatronInternalEdited,
    
    //A time-line seek changed the call, called when timeline time changes
    eValueChangedReasonTimeChanged ,
    
    //A master parameter ordered the slave to refresh its value
    eValueChangedReasonSlaveRefresh ,
    
    //The knob value has been restored to its defaults
    eValueChangedReasonRestoreDefault ,
};

enum AnimationLevelEnum
{
    eAnimationLevelNone = 0,
    eAnimationLevelInterpolatedValue = 1,
    eAnimationLevelOnKeyframe = 2
};

enum ImageComponentsEnum
{
    eImageComponentNone = 0,
    eImageComponentAlpha,
    eImageComponentRGB,
    eImageComponentRGBA,
    eImageComponentXY
};

enum ImagePremultiplicationEnum
{
    eImagePremultiplicationOpaque = 0,
    eImagePremultiplicationPremultiplied,
    eImagePremultiplicationUnPremultiplied,
};

enum ViewerCompositingOperatorEnum
{
    eViewerCompositingOperatorNone,
    eViewerCompositingOperatorOver,
    eViewerCompositingOperatorMinus,
    eViewerCompositingOperatorUnder,
    eViewerCompositingOperatorWipe
};

enum ViewerColorSpaceEnum
{
    eViewerColorSpaceSRGB = 0,
    eViewerColorSpaceLinear,
    eViewerColorSpaceRec709
};

enum ImageBitDepthEnum
{
    eImageBitDepthNone = 0,
    eImageBitDepthByte,
    eImageBitDepthShort,
    eImageBitDepthHalf,
    eImageBitDepthFloat
};

enum SequentialPreferenceEnum
{
    eSequentialPreferenceNotSequential = 0,
    eSequentialPreferenceOnlySequential,
    eSequentialPreferencePreferSequential
};

enum StorageModeEnum
{
    eStorageModeNone = 0, //< no memory will be allocated
    eStorageModeRAM, //< will be allocated in RAM using malloc or a malloc based implementation (such as std::vector)
    eStorageModeDisk //< will be allocated on virtual memory using mmap(). Fall-back on disk is assured by the operating system
};

enum OrientationEnum
{
    eOrientationHorizontal = 0x1,
    eOrientationVertical = 0x2
};
    
enum PlaybackModeEnum
{
    ePlaybackModeLoop = 0,
    ePlaybackModeBounce,
    ePlaybackModeOnce
};
    
enum SchedulingPolicyEnum
{
    eSchedulingPolicyFFA = 0, ///frames will be rendered concurrently without ordering (free for all)
    eSchedulingPolicyOrdered ///frames will be rendered in order
};
    
enum DisplayChannelsEnum
{
    eDisplayChannelsRGB = 0,
    eDisplayChannelsR,
    eDisplayChannelsG,
    eDisplayChannelsB,
    eDisplayChannelsA,
    eDisplayChannelsY,
    eDisplayChannelsMatte,
};
    
    /** @brief Enumerates the contexts a plugin can be used in */
enum ContextEnum
{
    eContextNone,
    eContextGenerator,
    eContextFilter,
    eContextTransition,
    eContextPaint,
    eContextGeneral,
    eContextRetimer,
    eContextReader,
    eContextWriter,
    eContextTracker
};
    
enum RotoStrokeType
{
    eRotoStrokeTypeSolid,
    eRotoStrokeTypeEraser,
    eRotoStrokeTypeClone,
    eRotoStrokeTypeReveal,
    eRotoStrokeTypeBlur,
    eRotoStrokeTypeSharpen,
    eRotoStrokeTypeSmear,
    eRotoStrokeTypeDodge,
    eRotoStrokeTypeBurn,
};


  
enum RenderSafetyEnum
{
    eRenderSafetyUnsafe = 0,
    eRenderSafetyInstanceSafe = 1,
    eRenderSafetyFullySafe = 2,
    eRenderSafetyFullySafeFrame = 3,
};
    
enum PenType
{
    ePenTypePen,
    ePenTypeCursor,
    ePenTypeEraser
};
    
enum PluginOpenGLRenderSupport {
    ePluginOpenGLRenderSupportNone, // None
    ePluginOpenGLRenderSupportYes, // Can do both CPU or GPU
    ePluginOpenGLRenderSupportNeeded // Can do only GPU
};

enum CurveChangeReason
{
     eCurveChangeReasonCurveEditor,
     eCurveChangeReasonDopeSheet,
     eCurveChangeReasonInternal
};
    
enum DopeSheetItemType
{
    eDopeSheetItemTypeCommon = 1001,
    
    // Range-based nodes
    eDopeSheetItemTypeReader,
    eDopeSheetItemTypeRetime,
    eDopeSheetItemTypeTimeOffset,
    eDopeSheetItemTypeFrameRange,
    eDopeSheetItemTypeGroup,
    
    // Others
    eDopeSheetItemTypeKnobRoot,
    eDopeSheetItemTypeKnobDim
};
    
}
Q_DECLARE_METATYPE(Natron::StandardButtons)


#endif // NATRON_GLOBAL_ENUMS_H
