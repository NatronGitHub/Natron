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
enum Scale_Type
{
    LINEAR_SCALE,LOG_SCALE
};

enum TIMELINE_STATE
{
    IDLE,DRAGGING_CURSOR,DRAGGING_BOUNDARY
};

enum TIMELINE_CHANGE_REASON
{
    USER_SEEK = 0, PLAYBACK_SEEK = 1, CURVE_EDITOR_SEEK = 2,
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

enum Status
{
    StatOK = 0,
    StatFailed = 1,
    StatReplyDefault = 14
};

/*Copy of QMessageBox::StandardButton*/
enum StandardButton
{
    NoButton           = 0x00000000,
    Escape             = 0x00000200,            // obsolete
    Ok                 = 0x00000400,
    Save               = 0x00000800,
    SaveAll            = 0x00001000,
    Open               = 0x00002000,
    Yes                = 0x00004000,
    YesToAll           = 0x00008000,
    No                 = 0x00010000,
    NoToAll            = 0x00020000,
    Abort              = 0x00040000,
    Retry              = 0x00080000,
    Ignore             = 0x00100000,
    Close              = 0x00200000,
    Cancel             = 0x00400000,
    Discard            = 0x00800000,
    Help               = 0x01000000,
    Apply              = 0x02000000,
    Reset              = 0x04000000,
    RestoreDefaults    = 0x08000000
};

typedef QFlags<Natron::StandardButton> StandardButtons;

enum MessageType
{
    INFO_MESSAGE = 0,ERROR_MESSAGE = 1,WARNING_MESSAGE = 2,QUESTION_MESSAGE = 3
};

enum KeyframeType
{
    KEYFRAME_CONSTANT = 0,
    KEYFRAME_LINEAR = 1,
    KEYFRAME_SMOOTH = 2,
    KEYFRAME_CATMULL_ROM = 3,
    KEYFRAME_CUBIC = 4,
    KEYFRAME_HORIZONTAL = 5,
    KEYFRAME_FREE = 6,
    KEYFRAME_BROKEN = 7,
    KEYFRAME_NONE = 8
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
enum ValueChangedReason
{
    //A user change to the knob triggered the call, gui will not be refreshed but instanceChangedAction called
    USER_EDITED = 0,
    
    //A plugin change triggered the call, gui will be refreshed but instanceChangedAction not called
    PLUGIN_EDITED ,
    
    // Natron gui called setValue itself, instanceChangedAction will be called (with a reason of User edited) AND knob gui refreshed
    NATRON_GUI_EDITED,
    
    // Natron engine called setValue itself, instanceChangedAction will be called (with a reason of plugin edited) AND knob gui refreshed
    NATRON_INTERNAL_EDITED,
    
    //A time-line seek changed the call, called when timeline time changes
    TIME_CHANGED ,
    
    //A master parameter ordered the slave to refresh its value
    SLAVE_REFRESH ,
    
    //The knob value has been restored to its defaults
    RESTORE_DEFAULT ,
};

enum AnimationLevel
{
    NO_ANIMATION = 0,
    INTERPOLATED_VALUE = 1,
    ON_KEYFRAME = 2
};

enum ImageComponents
{
    ImageComponentNone = 0,
    ImageComponentAlpha,
    ImageComponentRGB,
    ImageComponentRGBA
};

enum ImagePremultiplication
{
    ImageOpaque = 0,
    ImagePremultiplied,
    ImageUnPremultiplied,
};

enum ViewerCompositingOperator
{
    OPERATOR_NONE,
    OPERATOR_OVER,
    OPERATOR_MINUS,
    OPERATOR_UNDER,
    OPERATOR_WIPE
};

enum ViewerColorSpace
{
    sRGB = 0,
    Linear,
    Rec709
};

enum ImageBitDepth
{
    IMAGE_NONE = 0,
    IMAGE_BYTE,
    IMAGE_SHORT,
    IMAGE_FLOAT
};

enum SequentialPreference
{
    EFFECT_NOT_SEQUENTIAL = 0,
    EFFECT_ONLY_SEQUENTIAL,
    EFFECT_PREFER_SEQUENTIAL
};

enum StorageMode
{
    NO_STORAGE = 0, //< no memory will be allocated
    RAM, //< will be allocated in RAM using malloc or a malloc based implementation (such as std::vector)
    DISK //< will be allocated on virtual memory using mmap(). Fall-back on disk is assured by the operating system
};

enum Orientation
{
    Horizontal = 0x1,
    Vertical = 0x2
};
    
enum PlaybackMode
{
    PLAYBACK_LOOP = 0,
    PLAYBACK_BOUNCE,
    PLAYBACK_ONCE
};
    
enum SchedulingPolicy
{
    SCHEDULING_FFA = 0, ///frames will be rendered concurrently without ordering (free for all)
    SCHEDULING_ORDERED ///frames will be rendered in order
};
    
}
Q_DECLARE_METATYPE(Natron::StandardButtons)


#endif // NATRON_GLOBAL_ENUMS_H_
