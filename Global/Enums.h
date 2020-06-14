/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2020 The Natron developers
 * (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
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

CLANG_DIAG_OFF(deprecated)
#include <QtCore/QMetaType>
CLANG_DIAG_ON(deprecated)

NATRON_NAMESPACE_ENTER
#ifdef SBK_RUN
// shiboken doesn't generate SbkNatronEngine_StandardButtonEnum_as_number unless it is put in a class or namespace
NATRON_NAMESPACE_EXIT
namespace NATRON_NAMESPACE {
#endif


class Flag
{
        int i;
    public:
        inline Flag(int i);
        inline operator int() const { return i; }
};

inline Flag::Flag(int ai) : i(ai) {}

class IncompatibleFlag
{
    int i;
    public:
    inline explicit IncompatibleFlag(int i);
    inline operator int() const { return i; }
};
    
inline IncompatibleFlag::IncompatibleFlag(int ai) : i(ai) {}


template<typename Enum>
class Flags
{
    typedef void **Zero;
    int i;
public:
    typedef Enum enum_type;
    inline Flags(const Flags &f) : i(f.i) {}
    inline Flags(Enum f) : i(f) {}
    inline Flags(Zero = 0) : i(0) {}
    inline Flags(Flag f) : i(f) {}

    inline Flags &operator=(const Flags &f) { i = f.i; return *this; }
    inline Flags &operator&=(int mask) { i &= mask; return *this; }
    inline Flags &operator&=(uint mask) { i &= mask; return *this; }
    inline Flags &operator|=(Flags f) { i |= f.i; return *this; }
    inline Flags &operator|=(Enum f) { i |= f; return *this; }
    inline Flags &operator^=(Flags f) { i ^= f.i; return *this; }
    inline Flags &operator^=(Enum f) { i ^= f; return *this; }

    inline operator int() const { return i; }

    inline Flags operator|(Flags f) const { return Flags(Enum(i | f.i)); }
    inline Flags operator|(Enum f) const { return Flags(Enum(i | f)); }
    inline Flags operator^(Flags f) const { return Flags(Enum(i ^ f.i)); }
    inline Flags operator^(Enum f) const { return Flags(Enum(i ^ f)); }
    inline Flags operator&(int mask) const { return Flags(Enum(i & mask)); }
    inline Flags operator&(uint mask) const { return Flags(Enum(i & mask)); }
    inline Flags operator&(Enum f) const { return Flags(Enum(i & f)); }
    inline Flags operator~() const { return Flags(Enum(~i)); }

    inline bool operator!() const { return !i; }

    inline bool testFlag(Enum f) const { return (i & f) == f && (f != 0 || i == int(f) ); }
};

#define DECLARE_FLAGS(f, Enum)\
typedef Flags<Enum> f;

#define DECLARE_INCOMPATIBLE_FLAGS(f) \
inline NATRON_NAMESPACE::IncompatibleFlag operator|(f::enum_type f1, int f2) \
{ return NATRON_NAMESPACE::IncompatibleFlag(int(f1) | f2); }

#define DECLARE_OPERATORS_FOR_FLAGS(f) \
inline NATRON_NAMESPACE::Flags<f::enum_type> operator|(f::enum_type f1, f::enum_type f2) \
{ return NATRON_NAMESPACE::Flags<f::enum_type>(f1) | f2; } \
inline NATRON_NAMESPACE::Flags<f::enum_type> operator|(f::enum_type f1, NATRON_NAMESPACE::Flags<f::enum_type> f2) \
{ return f2 | f1; } DECLARE_INCOMPATIBLE_FLAGS(f)

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
    eTimelineChangeReasonAnimationModuleSeek = 2,
    eTimelineChangeReasonOtherSeek
};

enum ActionRetCodeEnum
{
    // Everything went ok, the operation completed successfully
    eActionStatusOK = 0,

    // Something failed, the plug-in is expected to post an error message
    // with setPersistentMessage
    eActionStatusFailed,

    // The render failed because a mandatory input of a node is diconnected
    // In this case there's no need for a persistent message, a black image is enough
    eActionStatusInputDisconnected,

    // The render was aborted, everything should abort ASAP and
    // the UI should not be updated with the processed images
    eActionStatusAborted,

    // The action failed because of a lack of memory.
    // If the action is using a GPU backend, it may re-try the same action on CPU right away
    eActionStatusOutOfMemory,

    // The operation completed with default implementation
    eActionStatusReplyDefault
};

inline bool isFailureRetCode(ActionRetCodeEnum code)
{
    switch (code) {
        case eActionStatusAborted:
        case eActionStatusFailed:
        case eActionStatusOutOfMemory:
        case eActionStatusInputDisconnected:
            return true;
        case eActionStatusOK:
        case eActionStatusReplyDefault:
            return false;
    }
    return true;
}



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
    NATRON_PIXMAP_PLAYER_STOP_DISABLED,
    NATRON_PIXMAP_PLAYER_STOP_ENABLED,
    NATRON_PIXMAP_PLAYER_PAUSE_DISABLED,
    NATRON_PIXMAP_PLAYER_PAUSE_ENABLED,
    NATRON_PIXMAP_PLAYER_LOOP_MODE,
    NATRON_PIXMAP_PLAYER_BOUNCE,
    NATRON_PIXMAP_PLAYER_PLAY_ONCE,
    NATRON_PIXMAP_PLAYER_TIMELINE_IN,
    NATRON_PIXMAP_PLAYER_TIMELINE_OUT,

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
    NATRON_PIXMAP_VIEWER_FULL_FRAME_ON,
    NATRON_PIXMAP_VIEWER_FULL_FRAME_OFF,
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
    NATRON_PIXMAP_ADD_USER_KEY,
    NATRON_PIXMAP_REMOVE_USER_KEY,
    NATRON_PIXMAP_SHOW_TRACK_ERROR,
    NATRON_PIXMAP_HIDE_TRACK_ERROR,
    NATRON_PIXMAP_RESET_TRACK_OFFSET,
    NATRON_PIXMAP_CREATE_USER_KEY_ON_MOVE_ON,
    NATRON_PIXMAP_CREATE_USER_KEY_ON_MOVE_OFF,
    NATRON_PIXMAP_RESET_USER_KEYS,
    NATRON_PIXMAP_CENTER_VIEWER_ON_TRACK,
    NATRON_PIXMAP_TRACK_BACKWARD_ON,
    NATRON_PIXMAP_TRACK_BACKWARD_OFF,
    NATRON_PIXMAP_TRACK_FORWARD_ON,
    NATRON_PIXMAP_TRACK_FORWARD_OFF,
    NATRON_PIXMAP_TRACK_PREVIOUS,
    NATRON_PIXMAP_TRACK_NEXT,
    NATRON_PIXMAP_TRACK_RANGE,
    NATRON_PIXMAP_TRACK_ALL_KEYS,
    NATRON_PIXMAP_TRACK_CURRENT_KEY,
    NATRON_PIXMAP_NEXT_USER_KEY,
    NATRON_PIXMAP_PREV_USER_KEY,

    NATRON_PIXMAP_ENTER_GROUP,

    NATRON_PIXMAP_SETTINGS,
    NATRON_PIXMAP_FREEZE_ENABLED,
    NATRON_PIXMAP_FREEZE_DISABLED,

    NATRON_PIXMAP_CURVE_EDITOR,
    NATRON_PIXMAP_DOPE_SHEET,
    NATRON_PIXMAP_NODE_GRAPH,
    NATRON_PIXMAP_PROGRESS_PANEL,
    NATRON_PIXMAP_SCRIPT_EDITOR,
    NATRON_PIXMAP_PROPERTIES_PANEL,
    NATRON_PIXMAP_ANIMATION_MODULE,
    NATRON_PIXMAP_VIEWER_PANEL,

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
    NATRON_PIXMAP_LINK_MULT_CURSOR,

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
    eValueChangedReasonPluginEdited,

    //A time-line seek changed the call, called when timeline time changes
    eValueChangedReasonTimeChanged,

    //The knob value has been restored to its defaults
    eValueChangedReasonRestoreDefault,
};

// Controls the return value of a setValue call on a knob
enum ValueChangedReturnCodeEnum
{
    eValueChangedReturnCodeNoKeyframeAdded = 0,
    eValueChangedReturnCodeKeyframeModified,
    eValueChangedReturnCodeKeyframeAdded,
    eValueChangedReturnCodeNothingChanged,
};


// Used to add stretch before or after widgets
enum ViewerContextLayoutTypeEnum
{
    eViewerContextLayoutTypeSpacing,
    eViewerContextLayoutTypeSeparator,
    eViewerContextLayoutTypeStretchAfter,
    eViewerContextLayoutTypeAddNewLine
};

// Controls how the knob contributes to the hash of a node.
enum KnobFrameViewHashingStrategyEnum
{
    // The value (for each dimension) at the current frame/view is added to the hash
    eKnobHashingStrategyValue,

    // All keyframes of the curve for all dimensions are added to the cache.
    // Typically this is needed for animated parameters that have an effect overtime which is
    // integrated such as a speed param of a retimer: the speed could be 5 at frame 1 and 10 at frame 100
    // but if the speed changes to 6 at frame 1, then output at frame 100 should change as well
    eKnobHashingStrategyAnimation
};

enum AnimationLevelEnum
{
    eAnimationLevelNone = 0,
    eAnimationLevelInterpolatedValue = 1,
    eAnimationLevelOnKeyframe = 2,
    eAnimationLevelExpression = 3
};

enum ImagePlaneDescEnum
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

enum ImageFieldingOrderEnum
{
    eImageFieldingOrderNone, // no fielding
    eImageFieldingOrderLower, // rows 0, 2 ...
    eImageFieldingOrderUpper, // rows 1, 3 ...
};

enum ImageFieldExtractionEnum
{
    // fetch a full frame interlaced image
    eImageFieldExtractionBoth,

    // fetch a single field, making a half height image
    eImageFieldExtractionSingle,

    // fetch a single field, but doubling each line and so making a full height image
    eImageFieldExtractionDouble
};

enum ViewerCompositingOperatorEnum
{
    eViewerCompositingOperatorNone,
    eViewerCompositingOperatorWipeUnder,
    eViewerCompositingOperatorWipeOver,
    eViewerCompositingOperatorWipeMinus,
    eViewerCompositingOperatorWipeOnionSkin,
    eViewerCompositingOperatorStackUnder,
    eViewerCompositingOperatorStackOver,
    eViewerCompositingOperatorStackMinus,
    eViewerCompositingOperatorStackOnionSkin,
};

enum ViewerColorSpaceEnum
{
    eViewerColorSpaceLinear = 0,
    eViewerColorSpaceSRGB,
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

enum CacheAccessModeEnum
{
    // The image should not use the cache at all
    eCacheAccessModeNone,

    // The image shoud try to look for a match in the cache
    // if possible. Tiles that are allocated or modified will be
    // pushed to the cache.
    eCacheAccessModeReadWrite,

    // The image should use cached tiles but can write to the cache
    eCacheAccessModeWriteOnly
};

enum ImageBufferLayoutEnum
{
    // This will make an image with an internal storage composed
    // of a single buffer for each channel.
    // This is the preferred storage format for the Cache.
    eImageBufferLayoutMonoChannelFullRect,


    // This will make an image with an internal storage composed of a single
    // buffer for all r g b and a channels. The buffer is layout as such:
    // all red pixels first, then all greens, then blue then alpha:
    // RRRRRRGGGGGBBBBBAAAAA
    eImageBufferLayoutRGBACoplanarFullRect,

    // This will make an image with an internal storage composed of a single
    // packed RGBA buffer. Pixels layout is as such: RGBARGBARGBA
    // This is the preferred layout by default for OpenFX.
    // OpenGL textures only support this mode for now.
    eImageBufferLayoutRGBAPackedFullRect,
};

enum RenderBackendTypeEnum
{
    // Render is done on CPU, this is the default
    eRenderBackendTypeCPU,

    // GPU OpenGL: the image is an OpenGL texture
    eRenderBackendTypeOpenGL,

    // CPU OpenGL with OSMesa (only if plug-ins returns true to canCPUImplementationSupportOSMesa())
    // The image is a RAM image but is bound to the default framebuffer and the plug-in can do an OpenGL render
    eRenderBackendTypeOSMesa
};

enum RenderScaleSupportEnum
{
    eSupportsMaybe = -1, // We don't know yet if the effect supports render scale
    eSupportsNo = 0, // Does not support, the effect can only render at scale 1
    eSupportsYes = 1 // Supports it, the effect can render at any scale
};

enum StorageModeEnum
{
    eStorageModeNone = 0, //< no memory will be allocated
    eStorageModeRAM, //< will be allocated in RAM using malloc or a malloc based implementation (such as std::vector) or mmap
    eStorageModeGLTex //< will be allocated as an OpenGL texture
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

enum RenderDirectionEnum
{
    eRenderDirectionForward = 0,
    eRenderDirectionBackward
};
    


enum DisplayChannelsEnum
{
    eDisplayChannelsY = 0,
    eDisplayChannelsRGB,
    eDisplayChannelsR,
    eDisplayChannelsG,
    eDisplayChannelsB,
    eDisplayChannelsA,
    eDisplayChannelsMatte
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
    eRotoStrokeTypeComp
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
    ePenTypeLMB,
    ePenTypeMMB,
    ePenTypeRMB,
    ePenTypePen,
    ePenTypeCursor,
    ePenTypeEraser
};

enum PluginOpenGLRenderSupport
{
    ePluginOpenGLRenderSupportNone, // None
    ePluginOpenGLRenderSupportYes, // Can do both CPU or GPU
    ePluginOpenGLRenderSupportNeeded // Can do only GPU
};

/**
* @brief Controls how planes are handled by this node if it cannot produce them
**/
enum PlanePassThroughEnum
{
    // If a plane does not exist on this node, returns a black image
    ePassThroughBlockNonRenderedPlanes,

    // If a plane does not exist on this node, ask it on the pass-through input returned by getLayersProducedAndNeeded_public
    ePassThroughPassThroughNonRenderedPlanes,

    // We render any plane, this is the same as if isMultiPlanar() == false
    ePassThroughRenderAllRequestedPlanes
};

/**
* @brief Controls how views are rendered by this effect
**/
enum ViewInvarianceLevel
{
    // All views produce a different result and will have a separate cache entry
    eViewInvarianceAllViewsVariant,

    // Unused
    eViewInvarianceOnlyPassThroughPlanesVariant,

    // All views are assumed to produce the same result hence they all share the same cache entry
    eViewInvarianceAllViewsInvariant,
};


enum OpenGLRequirementsTypeEnum
{
    eOpenGLRequirementsTypeViewer,
    eOpenGLRequirementsTypeRendering
};

enum AnimatedItemTypeEnum
{
    eAnimatedItemTypeCommon = 1001,

    // Range-based nodes
    eAnimatedItemTypeReader,
    eAnimatedItemTypeRetime,
    eAnimatedItemTypeTimeOffset,
    eAnimatedItemTypeFrameRange,
    eAnimatedItemTypeGroup,

    // Others
    eAnimatedItemTypeKnobRoot,
    eAnimatedItemTypeKnobView,
    eAnimatedItemTypeKnobDim,

    eAnimatedItemTypeTableItemRoot
};

enum CreateNodeReason
{
    //The user created the node via the GUI
    eCreateNodeReasonUserCreate,

    //The user created the node via copy/paste
    eCreateNodeReasonCopyPaste,

    //The node was created following a project load
    eCreateNodeReasonProjectLoad,

    //The node was created by Natron internally
    eCreateNodeReasonInternal,
};

enum ExpressionLanguageEnum
{
    eExpressionLanguageExprTk,
    eExpressionLanguagePython
};

enum KnobClipBoardType
{
    eKnobClipBoardTypeCopyValue,
    eKnobClipBoardTypeCopyAnim,
    eKnobClipBoardTypeCopyLink,
    eKnobClipBoardTypeCopyExpressionLink,
    eKnobClipBoardTypeCopyExpressionMultCurveLink,
};

enum ValueIsNormalizedEnum
{
    eValueIsNormalizedNone = 0, ///< indicating that the dimension holds a  non-normalized value.
    eValueIsNormalizedX, ///< indicating that the dimension holds a value normalized against the X dimension of the project format
    eValueIsNormalizedY ///< indicating that the dimension holds a value normalized against the Y dimension of the project format
};

enum CursorEnum
{
    eCursorDefault,
    eCursorBlank,
    eCursorArrow,
    eCursorUpArrow,
    eCursorCross,
    eCursorIBeam,
    eCursorWait,
    eCursorBusy,
    eCursorForbidden,
    eCursorPointingHand,
    eCursorWhatsThis,
    eCursorSizeVer,
    eCursorSizeHor,
    eCursorBDiag,
    eCursorFDiag,
    eCursorSizeAll,
    eCursorSplitV,
    eCursorSplitH,
    eCursorOpenHand,
    eCursorClosedHand
};

///Keep this in sync with @openfx-supportext/ofxsMerging.h
enum MergingFunctionEnum
{
    eMergeATop = 0,
    eMergeAverage,
    eMergeColor,
    eMergeColorBurn,
    eMergeColorDodge,
    eMergeConjointOver,
    eMergeCopy,
    eMergeDifference,
    eMergeDisjointOver,
    eMergeDivide,
    eMergeExclusion,
    eMergeFreeze,
    eMergeFrom,
    eMergeGeometric,
    eMergeGrainExtract,
    eMergeGrainMerge,
    eMergeHardLight,
    eMergeHue,
    eMergeHypot,
    eMergeIn,
    // eMergeInterpolated,
    eMergeLuminosity,
    eMergeMask,
    eMergeMatte,
    eMergeMax,
    eMergeMin,
    eMergeMinus,
    eMergeMultiply,
    eMergeOut,
    eMergeOver,
    eMergeOverlay,
    eMergePinLight,
    eMergePlus,
    eMergeReflect,
    eMergeSaturation,
    eMergeScreen,
    eMergeSoftLight,
    eMergeStencil,
    eMergeUnder,
    eMergeXOR
};

enum TableChangeReasonEnum
{
    // The action was initiated by an internal call of the API of KnobItemsTable
    eTableChangeReasonInternal,

    // The action was initiated by an external call from the Viewer interface (overlay interacts)
    eTableChangeReasonViewer,

    // The action was initiated by calling the API of the table Gui
    eTableChangeReasonPanel
};


enum CurveTypeEnum
{
    eCurveTypeDouble = 0, //< the values held by the keyframes can be any real
    eCurveTypeInt, //< the values held by the keyframes can only be integers
    eCurveTypeChoice, //< same as eCurveTypeString, interpolation is restricted to constant and keyframes have a choice id property
    eCurveTypeProperties, //< generialization of eCurveTypeString: interpolation is constant and keyframes may have types properties
    eCurveTypeBool, //< the values held by the keyframes can be either 0 or 1 with constant interpolation
    eCurveTypeString //< the keyframes hold a string property and interpolation is restricted to constant
        // and times
};

enum SetKeyFrameFlagEnum
{
    // The value set to the keyframe is set on the interpolated value.
    // For string values, this is actually set to a property since it's not a PoD scalar value
    eSetKeyFrameFlagSetValue = 0x1,

    // If there's any existing keyframe, the properties of the existing keyframe will be replaced
    // by this keyframe, but not its value. This is incompatible with eKeyFrameSetModeMergeProperties
    eSetKeyFrameFlagReplaceProperties = 0x2,

    // If there's any existing keyframe, the properties of the existing keyframe will be merged with
    // the properties of this keyframe. Any existing property will be replaced the property of the new keyframe.
    // This is incompatible with eKeyFrameSetModeMergeProperties.
    eSetKeyFrameFlagMergeProperties = 0x4,
        
};

enum OverlayViewportTypeEnum
{
    // An overlay that is drawn in the viewer
    eOverlayViewportTypeViewer,

    // An overlay that is drawn in the timeline (not the animation module)
    eOverlayViewportTypeTimeline
};

enum OverlaySlaveParamFlag
{
    eOverlaySlaveViewport = 0x1,
    eOverlaySlaveTimeline = 0x2
};


DECLARE_FLAGS(SetKeyFrameFlags, SetKeyFrameFlagEnum);
DECLARE_FLAGS(StandardButtons, StandardButtonEnum);
DECLARE_FLAGS(OverlaySlaveParamFlags, OverlaySlaveParamFlag);


#ifdef SBK_RUN
}
NATRON_NAMESPACE_ENTER
#endif


NATRON_NAMESPACE_EXIT

DECLARE_OPERATORS_FOR_FLAGS(NATRON_NAMESPACE::SetKeyFrameFlags);
DECLARE_OPERATORS_FOR_FLAGS(NATRON_NAMESPACE::StandardButtons);
DECLARE_OPERATORS_FOR_FLAGS(NATRON_NAMESPACE::OverlaySlaveParamFlags);

Q_DECLARE_METATYPE(NATRON_NAMESPACE::StandardButtons)


#endif // NATRON_GLOBAL_ENUMS_H
