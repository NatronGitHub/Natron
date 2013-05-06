#ifndef INCLUDED_ENUMS_H
#define INCLUDED_ENUMS_H
#include <boost/cstdint.hpp>

#define MAX_CHANNEL_COUNT 17

namespace Powiter_Enums{
    
	typedef boost::uint32_t U32;
	typedef boost::uint8_t U8;
	typedef boost::uint16_t U16;
    
    
    
    enum Knob_Types{FILE_KNOB,CHANNELS_KNOB,INT_KNOB,FLOAT_KNOB,STRING_KNOB,BOOL_KNOB};
    
    enum Knob_Flags{INVISIBLE=0x1,READ_ONLY=0x2};
    
    enum File_Type{EXR,DPX,TIFF,JPEG,FFMPEG,PNG,OTHER};
    
    enum Scale_Type{LINEAR_SCALE,LOG_SCALE,EXP_SCALE};
    
    enum UI_NODE_TYPE{OUTPUT,INPUT_NODE,OPERATOR,UNDEFINED};
    
    enum TIMELINE_STATE{IDLE,DRAGGING_CURSOR,DRAGGING_BOUNDARY};
    
    enum Channel{
        Channel_black = 0,
        Channel_red=1,
        Channel_green=2,
        Channel_blue=3,
        Channel_alpha=4,
        Channel_Z=5,
        Channel_U=6,
        Channel_V=7,
        Channel_Backward_U=8,
        Channel_Backward_V=9,
        Channel_Stereo_Disp_Left_X=10,
        Channel_Stereo_Disp_Left_Y=11,
        Channel_Stereo_Disp_Right_X=12,
        Channel_Stereo_Disp_Right_Y=13,
        Channel_DeepFront=14,
        Channel_DeepBack=15,
        Channel_unused=16
    };
    
    
    
    enum ChannelSetInit {
        Mask_None  = 0,
        Mask_Red   = 1 << (Channel_red - 1),  //1
        Mask_Green = 1 << (Channel_green - 1), // 2
        Mask_Blue  = 1 << (Channel_blue - 1), //4
        Mask_Alpha = 1 << (Channel_alpha - 1), //8
        Mask_Z     = 1 << (Channel_Z - 1), //16
        
        Mask_DeepBack = 1 << (Channel_DeepBack - 1), 
        Mask_DeepFront = 1 << (Channel_DeepFront - 1),
        
        Mask_Deep = (Mask_DeepBack | Mask_DeepFront),
        
        Mask_U     = 1 << (Channel_U - 1),
        Mask_V     = 1 << (Channel_V - 1),
        Mask_Backward_U = 1 << (Channel_Backward_U - 1),
        Mask_Backward_V = 1 << (Channel_Backward_V - 1),
        
        Mask_Stereo_Disp_Left_X     = 1 << (Channel_Stereo_Disp_Left_X - 1),
        Mask_Stereo_Disp_Left_Y     = 1 << (Channel_Stereo_Disp_Left_Y - 1),
        Mask_Stereo_Disp_Right_X     = 1 << (Channel_Stereo_Disp_Right_X - 1),
        Mask_Stereo_Disp_Right_Y     = 1 << (Channel_Stereo_Disp_Right_Y - 1),
        
        Mask_RGB   = Mask_Red | Mask_Green | Mask_Blue, // 7
        Mask_RGBA  = Mask_RGB | Mask_Alpha, // 15
        Mask_UV    = Mask_U | Mask_V, // 17
        Mask_MoVecForward = Mask_U | Mask_V,
        Mask_MoVecBackward = Mask_Backward_U | Mask_Backward_V,
        Mask_MoVec = Mask_U | Mask_V | Mask_Backward_U | Mask_Backward_V,
        
        Mask_Stereo_Disp_Left = Mask_Stereo_Disp_Left_X | Mask_Stereo_Disp_Left_Y,
        Mask_Stereo_Disp_Right = Mask_Stereo_Disp_Right_X | Mask_Stereo_Disp_Right_Y,
        Mask_Stereo_Disp = Mask_Stereo_Disp_Left | Mask_Stereo_Disp_Right,
        
        Mask_Builtin = Mask_RGBA | Mask_Z | Mask_Deep | Mask_MoVec | Mask_Stereo_Disp ,
        
        Mask_All   = 0xFFFFFFFF
    };

    
    enum MMAPfile_mode{ReadOnly,ReadWrite,Priv};
    
    enum ROW_RANK{FIRST,LAST,ND};

    
}

#endif // INCLUDED_ENUMS_H
