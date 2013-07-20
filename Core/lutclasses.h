//  Powiter
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
*Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012. 
*contact: immarespond at gmail dot com
*
*/

 

 



#ifndef __SRGB_LUT_H__
#define __SRGB_LUT_H__

#include <cmath>
#include "Core/lookUpTables.h"

class sRGB:public Lut{
public:
    
    sRGB():Lut(){}
    static  float toSRGB(float v){
        if (v<=0.0031308f){
			return (12.92f*v);
		}else{
			return (((1.055f)*powf(v,1.f/2.4f))-0.055f);
		}
    }
	float to_byte(float v){
        //to SRGB
        if (v<=0.0031308f){
			return (12.92f*v)*255.f;
		}else{
			return (((1.055f)*powf(v,1.f/2.4f))-0.055f)*255.f;
		}
	}

    
	float from_byte(float v){
        // to linear
        v/=255.f;
		if(v<=0.04045f){
			return (v/12.92f);
		}else{
			return (powf((v+0.055f)/(1.055f),2.4f));
		}
	}
};
class Rec709:public Lut{
public:
    
    Rec709() : Lut(){}
    static float toREC709(float v){
        return v < 0.018f ? v*4.500f : (1.099f*powf(v,0.45f) - 0.099f);
    }
    float to_byte(float v){
        //to Rec709 :
        return v < 0.018f ? v*4.500f*255.f : (1.099f*powf(v,0.45f) - 0.099f)*255.f;   
    }
    float from_byte(float v){
        //to linear
        return v <= 0.081f ? (v/4.5f)*255.f : powf((v+0.099f)/1.099f,1.f/0.45f)*255.f;
    }
};

class LinearLut:public Lut{
public:
	float to_byte(float v){
		return v*255.f;
	}
	float from_byte(float v){
		return v/255.f;
	}
};
#endif //__SRGB_LUT_H__