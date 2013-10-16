//  Powiter
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
*Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012. 
*contact: immarespond at gmail dot com
*
*/

#ifndef POWITER_GLOBAL_ENUMS_H_
#define POWITER_GLOBAL_ENUMS_H_

#define POWITER_MAX_CHANNEL_COUNT 31 // ChannelSet can handle channels 1..31, so it must be 31
#define POWITER_MAX_VALID_CHANNEL_INDEX 6

namespace Powiter{
    
    
        
    enum Scale_Type{LINEAR_SCALE,LOG_SCALE,EXP_SCALE};
        
    enum TIMELINE_STATE{IDLE,DRAGGING_CURSOR,DRAGGING_BOUNDARY};

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
}



#endif // POWITER_GLOBAL_ENUMS_H_
