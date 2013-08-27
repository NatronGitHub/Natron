//  Powiter
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
*Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012. 
*contact: immarespond at gmail dot com
*
*/

 

 




#ifndef INCLUDED_ENUMS_H
#define INCLUDED_ENUMS_H

#define MAX_CHANNEL_COUNT 7

namespace Powiter{
    
    
        
    enum Scale_Type{LINEAR_SCALE,LOG_SCALE,EXP_SCALE};
        
    enum TIMELINE_STATE{IDLE,DRAGGING_CURSOR,DRAGGING_BOUNDARY};
    
    enum Channel{
        Channel_black = 0,
        Channel_red=1,
        Channel_green=2,
        Channel_blue=3,
        Channel_alpha=4,
        Channel_Z=5,
        Channel_unused=6
    };
    
    
    
    enum ChannelMask {
        Mask_None  = 0,
        Mask_Red   = 1 << (Channel_red - 1),  //1
        Mask_Green = 1 << (Channel_green - 1), // 2
        Mask_Blue  = 1 << (Channel_blue - 1), //4
        Mask_Alpha = 1 << (Channel_alpha - 1), //8
        Mask_Z     = 1 << (Channel_Z - 1), //16
        Mask_RGB   = Mask_Red | Mask_Green | Mask_Blue, // 7
        Mask_RGBA  = Mask_RGB | Mask_Alpha, // 15       
        Mask_All   = 0xFFFFFFFF
    };

    
    enum MMAPfile_mode{if_exists_fail_if_not_exists_create,
        if_exists_keep_if_dont_exists_fail,
        if_exists_keep_if_dont_exists_create,
        if_exists_truncate_if_not_exists_fail,
        if_exists_truncate_if_not_exists_create};
    
}



#endif // INCLUDED_ENUMS_H
