#ifndef __SRGB_LUT_H__
#define __SRGB_LUT_H__

#include <cmath>
#include "Core/lookUpTables.h"

class sRGB:public Lut{
public:
    
    sRGB():Lut(){}
    static  float toSRGB(float v){
        if (v<=0.0031308){
			return (12.92*v);
		}else{
			return (((1.055)*powf(v,1.f/2.4))-0.055);
		}
    }
	float to_byte(float v){
        //to SRGB
        if (v<=0.0031308){
			return (12.92*v)*255.f;
		}else{
			return (((1.055)*powf(v,1.f/2.4))-0.055)*255.f;
		}
	}

    
	float from_byte(float v){
        // to linear
        v/=255.f;
		if(v<=0.04045){
			return (v/12.92);
		}else{
			return (powf((v+0.055)/(1.055),2.4));
		}
	}
};
class Rec709:public Lut{
public:
    
    Rec709() : Lut(){}
    static float toREC709(float v){
        return v < 0.018 ? v*4.500 : (1.099*powf(v,0.45) - 0.099);
    }
    float to_byte(float v){
        //to Rec709 :
        return v < 0.018 ? v*4.500*255.f : (1.099*powf(v,0.45) - 0.099)*255.f;   
    }
    float from_byte(float v){
        //to linear
        return v <= 0.081 ? (v/4.5)*255.f : powf((v+0.099)/1.099,1.f/0.45)*255.f;
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