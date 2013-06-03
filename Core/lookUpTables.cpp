//  Powiter
//
//  Created by Alexandre Gauthier-Foichat on 06/12
//  Copyright (c) 2013 Alexandre Gauthier-Foichat. All rights reserved.
//  contact: immarespond at gmail dot com
#include <iostream>
#include <QtGui/QRgb>
#include "Core/lutclasses.h"

#ifndef FLT_MAX
# define FLT_MAX 3.40282347e+38F
#endif

using namespace std;
/*Fill in the table used by toFloatFast() and thus by all the to functions. This is done by calling to_byte().
 This can be used by a from_byte() implementation that works by using interpolation to invert the to_byte() function.
 Does nothing if called a second time. */
void Lut::fillToTable(){
    // float increment = 1.f /65535.f;
    if(init_) return;
    bool linearTrue = true;
    for (int i = 0; i < 0x10000; i++) {
        float inp = index_to_float(i);
        float f = to_byte(inp);
        if(!isEqual_float(f, inp*255.f, 10000))
            linearTrue = false;
        if (f <= 0) to_byte_table[i] = 0;
        else if (f < 255) to_byte_table[i] = (unsigned short)(f*0x100+.5);
        else to_byte_table[i] = 0xff00;
    }
    if (linearTrue) {
        linear_=true;
    }
    
}

/*Fill in the table used by fromFloatFast() and thus all the from functions.
 This is done by calling from_byte(). This can be used by a to_byte() implementation taht works by using interpolation to invert the from_byte() function.
 Does nothing if called a second time. */
void Lut::fillFromTable()
{
    if(init_) return;
    int i;
    for (int b = 0; b <= 255; b++) {
        float f[1]; f[0] = from_byte((float)b);
        from_byte_table[b] = f[0];
        i = highFloatPart(f);
        to_byte_table[i] = b*0x100;
    }
    //	for(int i=0;i<256;i++){
    //		from_byte_table[i]=from_byte(i);
    //	}
}

/*Forces fillToTable() and fillFromTable() to fill the tables again and calls them, and calculates a value for hash().
 The constructor calls this, but subclasses may want to call this directly if the LUT has other controls that can change it's results. */
void Lut::validate()
{
    if (!linear_) {
        fillToTable();
        fillFromTable();
    }
    init_=true;
	
}
float Lut::index_to_float(const U16 i)
{
    float f[1];
    unsigned short *s = (unsigned short *)f;
    /* positive and negative zeros, and all gradual underflow, turn into zero: */
    if (i<0x80 || (i >= 0x8000 && i < 0x8080)) return 0;
    /* All NaN's and infinity turn into the largest possible legal float: */
    if (i>=0x7f80 && i<0x8000) return FLT_MAX;
    if (i>=0xff80) return -FLT_MAX;
    if(_bigEndian){
        s[0] = i;
        s[1] = 0x8000;
    }else{
        s[0] = 0x8000;
        s[1] = i;
    }
    return f[0];
}

/*Converts a single floating point value to linear by using the lookup tables. The value is clamped to the 0-1 range as that is the range of the tables! */
float Lut::fromFloatFast(float v) {
	return from_byte_table[(int)v];
}
/*Converts a single floating point value from linear to the LUT space by using the lookup tables. */
float Lut::toFloatFast(float v)  {
    return (float)to_byte_table[highFloatPart(&v)];
}
void Lut::from_byte(float* to, const uchar* from, int W, int delta) {
    for(int i =0 ; i < W ; i+=delta){
        to[i]=from_byte_table[(int)from[i]];
    }
}

void Lut::from_byte(float* to, const uchar* from, const uchar* alpha, int W, int delta ) {
    for(int i =0 ; i < W ; i+=delta){
        float a = (float)alpha[i];
        to[i]=from_byte_table[(int)from[i]/(int)a] * a;
    }
}
void Lut::from_byteQt(float* to, const QRgb* from,Channel z,bool premult,int W,int delta) {
    typedef int(*ChannelLookup)(QRgb);
    ChannelLookup lookup;
    switch (z) {
        case Powiter_Enums::Channel_alpha:
            lookup = qAlpha;
            break;
        case Powiter_Enums::Channel_red:
            lookup = qRed;
            break;
        case Powiter_Enums::Channel_green:
            lookup = qGreen;
            break;
        case Powiter_Enums::Channel_blue:
            lookup = qBlue;
            break;
        default:
            lookup = qRed;
            break;
    }
    if(z == Powiter_Enums::Channel_alpha){
        Linear::from_byteQt(to, from, z, W,delta);
    }else{
        if(premult){
            for(int i =0 ; i < W ; i+=delta){
                const QRgb c = from[i];
                int alpha = qAlpha(c);
                to[i] = from_byte_table[((*lookup)(c)/alpha)] * alpha;
            }
        }else{
            for(int i =0 ; i < W ; i+=delta){
                const QRgb c = from[i];
                to[i] = from_byte_table[(*lookup)(c)];
            }
        }
    }
}

void Lut::from_short(float* to, const U16* from, int W, int bits , int delta ) {
    cout << "Lut::from_short not yet implemented" << endl;
}
void Lut::from_short(float* to, const U16* from, const U16* alpha, int W, int bits , int delta ) {
    cout << "Lut::from_short not yet implemented" << endl;
}
void Lut::from_float(float* to, const float* from, int W, int delta ) {
    for(int i=0;i< W;i+=delta){
        to[i]=fromFloatFast(from[i]*255.f);
    }
}
void Lut::from_float(float* to, const float* from, const float* alpha, int W, int delta) {
    for(int i=0;i< W;i+=delta){
        float a = alpha[i];
        to[i]=fromFloatFast((from[i]/a)*255.f) * a * 255.f;
    }
    
}

void Linear::from_byte(float* to, const uchar* from, int W, int delta ){
    for(int i =0;i < W ; i+=delta){
        to[i] = Linear::fromFloatFast(from[i]/255.f);
    }
}
void Linear::from_byteQt(float* to, const QRgb* from,Channel z, int W, int delta ){
    typedef int(*ChannelLookup)(QRgb);
    ChannelLookup lookup;
    switch (z) {
        case Powiter_Enums::Channel_alpha:
            lookup = qAlpha;
            break;
        case Powiter_Enums::Channel_red:
            lookup = qRed;
            break;
        case Powiter_Enums::Channel_green:
            lookup = qGreen;
            break;
        case Powiter_Enums::Channel_blue:
            lookup = qBlue;
            break;
        default:
            lookup = qRed;
            break;
    }
    if(z == Powiter_Enums::Channel_alpha){
        for(int i = 0 ; i < W ; i+=delta){
            const QRgb c = from[i];
            to[i] = fromFloatFast((float)qAlpha(c)/255.f);
        }
    }else{
        
        for(int i =0 ; i < W ; i+=delta){
            const QRgb c = from[i];
            to[i] = fromFloatFast((*lookup)(c));
        }
    }
    
}
void Linear::from_short(float* to, const U16* from, int W, int bits , int delta ){
     cout << "Linear::from_short not yet implemented" << endl;
}
void Linear::from_float(float* to, const float* from, int W, int delta ){
    for(int i=0;i< W;i+=delta){
        to[i]=fromFloatFast(from[i]);
    }
}
Lut* Lut::getLut(DataType type){
    switch (type) {
        case VIEWER:
            return new sRGB();
            break;
        case FLOAT:
            return Lut::Linear();
            break;
		case INT8:
			return new sRGB();
			break;
		case INT16:
			return new sRGB();
			break;
        case MONITOR:
            return new Rec709();
            break;
        default:
            return NULL;
            break;
    }
    
}

Lut* Lut::Linear(){
    Lut* lut=new LinearLut();
    lut->linear(true);
    return lut;
}