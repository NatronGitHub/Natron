//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
*Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012. 
*contact: immarespond at gmail dot com
*
*/

#ifndef NATRON_GLOBAL_ENUMS_H_
#define NATRON_GLOBAL_ENUMS_H_

#define NATRON_MAX_CHANNEL_COUNT 31 // ChannelSet can handle channels 1..31, so it must be 31
#define NATRON_MAX_VALID_CHANNEL_INDEX 6

#include <QFlags>
#include <QMessageBox>
#include <QMetaType>

namespace Natron{
    
    
        
    enum Scale_Type{LINEAR_SCALE,LOG_SCALE};
        
    enum TIMELINE_STATE{IDLE,DRAGGING_CURSOR,DRAGGING_BOUNDARY};
    
    enum TIMELINE_CHANGE_REASON{USER_SEEK = 0, PLAYBACK_SEEK = 1};

    // when adding more standard channels,
    // update the functions getChannelByName() and getChannelName() in ChannelSet.cpp
    enum Channel {
        Channel_black = 0,
        Channel_red=1,
        Channel_green=2,
        Channel_blue=3,
        Channel_alpha=4,
        Channel_Z=5,
        // note: Channel_unused is not a channel per se, it is just the first unused channel,
        // and its index may thus change between versions
        Channel_unused=6
    };
    
    
    
    enum ChannelMask {
        Mask_None  = 0,
        Mask_Red   = 1 << (Channel_red - 1),  //1
        Mask_Green = 1 << (Channel_green - 1), // 10
        Mask_Blue  = 1 << (Channel_blue - 1), // 100
        Mask_Alpha = 1 << (Channel_alpha - 1), // 1000
        Mask_Z     = 1 << (Channel_Z - 1), //10000
        Mask_RGB   = Mask_Red | Mask_Green | Mask_Blue, // 111
        Mask_RGBA  = Mask_RGB | Mask_Alpha, // 1111       
        Mask_All   = 0xFFFFFFFF
    };

    
    enum MMAPfile_mode{if_exists_fail_if_not_exists_create,
        if_exists_keep_if_dont_exists_fail,
        if_exists_keep_if_dont_exists_create,
        if_exists_truncate_if_not_exists_fail,
        if_exists_truncate_if_not_exists_create};
    
    enum Status{
        StatOK = 0,
        StatFailed = 1,
        StatReplyDefault = 14
    };
    
    /*Copy of QMessageBox::StandardButton*/
    enum StandardButton {
        NoButton           = 0x00000000,
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
    
    enum MessageType{INFO_MESSAGE = 0,ERROR_MESSAGE = 1,WARNING_MESSAGE = 2,QUESTION_MESSAGE = 3};

    
    typedef QFlags<QMessageBox::StandardButton> StandardButtons;

    enum KeyframeType {
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


    enum PixmapEnum{
        PLAYER_PREVIOUS_PIXMAP = 0,
        PLAYER_FIRST_FRAME_PIXMAP,
        PLAYER_NEXT_PIXMAP,
        PLAYER_LAST_FRAME_PIXMAP ,
        PLAYER_NEXT_INCR_PIXMAP ,
        PLAYER_NEXT_KEY_PIXMAP ,
        PLAYER_PREVIOUS_INCR_PIXMAP ,
        PLAYER_PREVIOUS_KEY_PIXMAP,
        PLAYER_REWIND_PIXMAP ,
        PLAYER_PLAY_PIXMAP ,
        PLAYER_STOP_PIXMAP ,
        PLAYER_LOOP_MODE_PIXMAP ,


        MAXIMIZE_WIDGET_PIXMAP ,
        MINIMIZE_WIDGET_PIXMAP ,
        CLOSE_WIDGET_PIXMAP,
        HELP_WIDGET_PIXMAP ,

        GROUPBOX_FOLDED_PIXMAP ,
        GROUPBOX_UNFOLDED_PIXMAP ,

        UNDO_PIXMAP ,
        REDO_PIXMAP ,
        UNDO_GRAYSCALE_PIXMAP,
        REDO_GRAYSCALE_PIXMAP ,

        TAB_WIDGET_LAYOUT_BUTTON_PIXMAP ,
        TAB_WIDGET_SPLIT_HORIZONTALLY_PIXMAP ,
        TAB_WIDGET_SPLIT_VERTICALLY_PIXMAP,

        VIEWER_CENTER_PIXMAP,
        VIEWER_CLIP_TO_PROJECT_PIXMAP,
        VIEWER_REFRESH_PIXMAP,

        OPEN_FILE_PIXMAP,
        RGBA_CHANNELS_PIXMAP,
        COLORWHEEL_PIXMAP,

        IO_GROUPING_PIXMAP,
        OPEN_EFFECTS_GROUPING_PIXMAP,
        
        COMBOBOX_PIXMAP,
        COMBOBOX_PRESSED_PIXMAP
    };

}
Q_DECLARE_METATYPE(Natron::StandardButtons)


#endif // NATRON_GLOBAL_ENUMS_H_
