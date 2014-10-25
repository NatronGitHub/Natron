//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 * Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 * contact: immarespond at gmail dot com
 *
 */

#ifndef NATRON_GLOBAL_ENUMS_H_
#define NATRON_GLOBAL_ENUMS_H_

#define NATRON_MAX_CHANNEL_COUNT 31 // ChannelSet can handle channels 1..31, so it must be 31
#define NATRON_MAX_VALID_CHANNEL_INDEX 6

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
};

enum TimelineChangeReasonEnum
{
    eTimelineChangeReasonUserSeek = 0,
    eTimelineChangeReasonPlaybackSeek = 1,
    eTimelineChangeReasonCurveEditorSeek = 2,
};

// when adding more standard channels,
// update the functions getChannelByName() and getChannelName() in ChannelSet.cpp
enum Channel
{
    Channel_black = 0,
    Channel_red = 1,
    Channel_green = 2,
    Channel_blue = 3,
    Channel_alpha = 4,
    Channel_Z = 5,
    // note: Channel_unused is not a channel per se, it is just the first unused channel,
    // and its index may thus change between versions
    Channel_unused = 6
};

enum ChannelMask
{
    Mask_None  = 0,
    Mask_Red   = 1 << (Channel_red - 1),      //1
        Mask_Green = 1 << (Channel_green - 1), // 10
        Mask_Blue  = 1 << (Channel_blue - 1), // 100
        Mask_Alpha = 1 << (Channel_alpha - 1), // 1000
        Mask_Z     = 1 << (Channel_Z - 1), //10000
        Mask_RGB   = Mask_Red | Mask_Green | Mask_Blue, // 111
        Mask_RGBA  = Mask_RGB | Mask_Alpha, // 1111
        Mask_All   = 0xFFFFFFFF
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
    NATRON_PIXMAP_VIEWER_ROI_ENABLED,
    NATRON_PIXMAP_VIEWER_ROI_DISABLED,
    NATRON_PIXMAP_VIEWER_RENDER_SCALE,
    NATRON_PIXMAP_VIEWER_RENDER_SCALE_CHECKED,

    NATRON_PIXMAP_OPEN_FILE,
    NATRON_PIXMAP_COLORWHEEL,
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

    NATRON_PIXMAP_BOLD_CHECKED,
    NATRON_PIXMAP_BOLD_UNCHECKED,
    NATRON_PIXMAP_ITALIC_CHECKED,
    NATRON_PIXMAP_ITALIC_UNCHECKED,

    NATRON_PIXMAP_CLEAR_ALL_ANIMATION,
    NATRON_PIXMAP_CLEAR_BACKWARD_ANIMATION,
    NATRON_PIXMAP_CLEAR_FORWARD_ANIMATION,
    NATRON_PIXMAP_UPDATE_VIEWER_ENABLED,
    NATRON_PIXMAP_UPDATE_VIEWER_DISABLED,
    
    NATRON_PIXMAP_SETTINGS,
    NATRON_PIXMAP_FREEZE_ENABLED,
    NATRON_PIXMAP_FREEZE_DISABLED,

    NATRON_PIXMAP_VIEWER_ICON,
    NATRON_PIXMAP_VIEWER_CHECKERBOARD_ENABLED,
    NATRON_PIXMAP_VIEWER_CHECKERBOARD_DISABLED,
    NATRON_PIXMAP_VIEWER_ZEBRA_ENABLED,
    NATRON_PIXMAP_VIEWER_ZEBRA_DISABLED,
    
    NATRON_PIXMAP_APP_ICON
};

///This enum is used when dealing with parameters which have their value edited
enum ValueChangedReasonEnum
{
    //A user change to the knob triggered the call, gui will not be refreshed but instanceChangedAction called
    eValueChangedReasonUserEdited = 0,
    
    //A plugin change triggered the call, gui will be refreshed but instanceChangedAction not called
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
    eImageComponentRGBA
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
    
}
Q_DECLARE_METATYPE(Natron::StandardButtons)


#endif // NATRON_GLOBAL_ENUMS_H_
