//  Powiter
//
//  Created by Alexandre Gauthier-Foichat on 06/12
//  Copyright (c) 2013 Alexandre Gauthier-Foichat. All rights reserved.
//  contact: immarespond at gmail dot com
#ifndef __LUT_CLASS_H__
#define __LUT_CLASS_H__

#include <QtCore/QMutex>
#include <QtCore/QThread>
#include <math.h>
#include "Superviser/powiterFn.h"
#include "Superviser/MemoryInfo.h"

using namespace Powiter_Enums;

/*This class implements : http://mysite.verizon.net/spitzak/conversion/algorithm.html*/

class Lut{
    U16 to_byte_table[0x10000]; // contains  2^16 = 65536 values
	float from_byte_table[256]; // values between 0-1.f.
	bool init_;
	bool linear_;
    bool _bigEndian;
            
protected:
    
    void fillToTable();
    void fillFromTable();
    
    
public:
    Lut() : init_(false), linear_(false) { _bigEndian = isBigEndian();}
    virtual ~Lut() {}
    
    bool linear() {  return linear_; }
    void linear(bool b){linear_=b;}
  
    static void allocateLuts();

    static void deallocateLuts();
    
    void validate();
    
    //takes in input a float clamped to [0 - 1.f] and  return
    // the value after color-space conversion is in [0 - 255.f]
    virtual float to_byte(float)  = 0;
    
    // takes in input a float clamped to [0 - 255.f] and do the math to convert it
    // to have the opposite effect of to_byte(float), returns it between [0-1f]
    virtual float from_byte(float)  = 0;

    void from_byte(float* to, const uchar* from, int W, int delta = 1) ;
    void from_byte(float* to, const uchar* from, const uchar* alpha, int W, int delta = 1) ;
    void from_byteQt(float* to, const QRgb* from,Channel z,bool premult,int W,int delta = 1) ;
    
    void from_short(float* to, const U16* from, int W, int bits = 16, int delta = 1) ;
    void from_short(float* to, const U16* from, const U16* alpha, int W, int bits = 16, int delta = 1) ;
    
    void from_float(float* to, const float* from, int W, int delta = 1) ;
    void from_float(float* to, const float* from, const float* alpha, int W, int delta = 1) ;
    
    float fromFloat(float v)  { return from_byte(v * 255.f); }
    float fromFloatFast(float v) ;
    float toFloat(float v)  { return to_byte(v) / 255.f; }
    float toFloatFast(float v) ;
    
    
    static  void U24_to_rgb(float *r,float *g,float *b,const uchar* ptr){
        *r = *ptr;
        *g = *(ptr+1);
        *b = *(ptr+2);
    }
    
	/*used to fill the to_byte lut*/
    float index_to_float(const U16 i);

    static U16 highFloatPart(float *f){return *((unsigned short*)f + 1);}

    static U16 lowFloatPart(float *f){return *((unsigned short*)f);}
    

    static float clamp(float v, float min = 0.f, float max= 1.f){
        if(v > max) v = max;
        if(v < min) v = min;
        return v;
    }
    static bool isEqual_float(float A, float B, int maxUlps)
    {
        // Make sure maxUlps is non-negative and small enough that the
        // default NAN won't compare as equal to anything.
        //maxUlps > 0 && maxUlps < 4 * 1024 * 1024);
        int aInt = *(int*)&A;
        // Make aInt lexicographically ordered as a twos-complement int
        if (aInt < 0)
            aInt = 0x80000000 - aInt;
        // Make bInt lexicographically ordered as a twos-complement int
        int bInt = *(int*)&B;
        if (bInt < 0)
            bInt = 0x80000000 - bInt;
        int intDiff = abs(aInt - bInt);
        if (intDiff <= maxUlps)
            return true;
        return false;
    }
    static Lut* Linear();
    
//
    // luts : linear,srgb,rec709,cineon,gamma 1.8,gamma2.2,panalog,redlog,viperlog,alexaV3logC,ploglin,slog,redspace
    // default : monitor = srgb,8bit :srgb, 16 bit:srgb, log :cineon,float : linear
    enum DataType { MONITOR = 0, VIEWER=1, INT8=2, INT16=3, LOG=4, FLOAT=5, GAMMA1_8=6, GAMMA2_2=7,
         PANALOG=8, REDLOG=9, VIPERLOG=10, ALEXAV3LOGC=11, PLOGLIN=12, SLOG=13, TYPES_END=14 };
//     // Return a LUT associated to the datatype
    static Lut* getLut(DataType);
    

    
private:

    static std::map<DataType,Lut*> _luts;

//     //! Modify above table
//     static void setLut(DataType, Lut*);
};

class Linear
{
public:

	// OPTIMIZATION : Make similar functions but taking out ptrs to r,g,b,a so copy is done all at once
	static float from_byte(float f) { return f * (1.0f / 255.0f); }

    static void from_byte(float* to, const uchar* from, int W, int delta = 1);
    static void from_byteQt(float* to, const QRgb* from,Channel z, int W, int delta = 1);
    static void from_short(float* to, const U16* from, int W, int bits = 16, int delta = 1);
    static void from_float(float* to, const float* from, int W, int delta = 1);
    
	static float to_byte(float f) { return f * 255.0f; }
    
	static float fromFloat(float v) { return v; }
    static float fromFloatFast(float v) { return v; }
	static float toFloat(float v) { return v; }
    static float toFloatFast(float v) { return v; }
    
};


#endif //__LUT_CLASS_H__
