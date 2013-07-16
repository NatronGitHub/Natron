//  Powiter
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
//  contact: immarespond at gmail dot com

/*
 *
 * High-speed conversion between 8 bit and floating point image data.
 *
 * Copyright 2002 Bill Spitzak and Digital Domain, Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA.
 *
 * For use in closed-source software please contact Digital Domain,
 * 300 Rose Avenue, Venice, CA 90291 310-314-2800
 *
 */


#ifndef __LUT_CLASS_H__
#define __LUT_CLASS_H__

#include <QtCore/QMutex>
#include <QtCore/QThread>
#include <math.h>
#include "Superviser/powiterFn.h"
#include "Superviser/MemoryInfo.h"
#include <QtGui/QRgb>
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
    
    float toFloat(float v)  { return to_byte(v) / 255.f; }
    float toFloatFast(float v) ;
    
    /*! Convert an array of linear floating point pixel values to an
     array of destination lut values, with error diffusion to avoid posterizing
     artifacts.
     
     \a W is the number of pixels to convert.  \a delta is the distance
     between the output bytes (useful for interlacing them into a buffer
     for screen display).
     
     The input and output buffers must not overlap in memory.
     */
    void to_byte(uchar* to, const float* from, int W, int delta = 1) ;    
    /*used to pre-multiply by alpha*/
    void to_byte(uchar* to, const float* from, const float* alpha, int W,int delta = -1);
    
    void to_short(U16* to, const float* from, int W, int bits = 16, int delta = 1);
    void to_short(U16* to, const float* from, const float* alpha, int W, int bits = 16, int delta = 1);
    
    void to_float(float* to, const float* from, int W, int delta = 1);
    void to_float(float* to, const float* from, const float* alpha, int W, int delta = 1);
    
    
    
    // takes in input a float clamped to [0 - 255.f] and do the math to convert it
    // to have the opposite effect of to_byte(float), returns it between [0 - 1.f]
    virtual float from_byte(float)  = 0;

    float fromFloat(float v)  { return from_byte(v * 255.f); }
    float fromFloatFast(float v) ;
    
    void from_byte(float* to, const uchar* from, int W, int delta = 1) ;
    void from_byte(float* to, const uchar* from, const uchar* alpha, int W, int delta = 1) ;
    void from_byteQt(float* to, const QRgb* from,Channel z,bool premult,int W,int delta = 1) ;
    
    void from_short(float* to, const U16* from, int W, int bits = 16, int delta = 1) ;
    void from_short(float* to, const U16* from, const U16* alpha, int W, int bits = 16, int delta = 1) ;
    
    void from_float(float* to, const float* from, int W, int delta = 1) ;
    void from_float(float* to, const float* from, const float* alpha, int W, int delta = 1) ;
    
    
    static void U24_to_rgb(float *r,float *g,float *b,const uchar* ptr){
        *r = *ptr;
        *g = *(ptr+1);
        *b = *(ptr+2);
    }
    
	/*used to fill the to_byte lut*/
    float index_to_float(const U16 i);

    static U16 highFloatPart(const float *f){return *((unsigned short*)f + 1);}

    static U16 lowFloatPart(const float *f){return *((unsigned short*)f);}
    

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

	static float from_byte(float f) { return f * (1.0f / 255.0f); }

    static void from_byte(float* to, const uchar* from, int W, int delta = 1);
    static void from_byteQt(float* to, const QRgb* from,Channel z, int W, int delta = 1);
    static void from_short(float* to, const U16* from, int W, int bits = 16, int delta = 1);
    static void from_float(float* to, const float* from, int W, int delta = 1);
    
	static float to_byte(float f) { return f * 255.0f; }
    
    /*! Convert an array of floating point pixel values to an array of
     bytes by multiplying by 255 and doing error diffusion to avoid
     banding. This should be used to convert mattes and alpha channels.
     
     \a W is the number of pixels to convert.  \a delta is the distance
     between the output bytes (useful for interlacing them into a buffer
     for screen display).
     
     The input and output buffers must not overlap in memory.
     */
    static void to_byte(uchar* to, const float* from, int W, int delta = 1);
    static void to_short(U16* to, const float* from, int W, int bits = 16, int delta = 1);
    static void to_float(float* to, const float* from, int W, int delta = 1);
    
	static float fromFloat(float v) { return v; }
    static float fromFloatFast(float v) { return v; }
	static float toFloat(float v) { return v; }
    static float toFloatFast(float v) { return v; }
    
};


#endif //__LUT_CLASS_H__
