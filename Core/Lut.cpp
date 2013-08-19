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

 

 



#include <iostream>
#include "Core/Lut.h"
#ifndef FLT_MAX
# define FLT_MAX 3.40282347e+38F
#endif

using namespace std;
using namespace Powiter;
std::map<Lut::DataType,Lut*> Lut::_luts;


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

}

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


float Lut::fromFloatFast(float v) {
	return from_byte_table[(int)v];
}
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
        case Powiter::Channel_alpha:
            lookup = qAlpha;
            break;
        case Powiter::Channel_red:
            lookup = qRed;
            break;
        case Powiter::Channel_green:
            lookup = qGreen;
            break;
        case Powiter::Channel_blue:
            lookup = qBlue;
            break;
        default:
            lookup = qRed;
            break;
    }
    if(z == Powiter::Channel_alpha){
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
    (void)to;
    (void)from;
    (void)W;
    (void)bits;
    (void)delta;
    cout << "Lut::from_short not yet implemented" << endl;
}
void Lut::from_short(float* to, const U16* from, const U16* alpha, int W, int bits , int delta ) {
    (void)to;
    (void)from;
    (void)W;
    (void)bits;
    (void)alpha;
    (void)delta;
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


void Lut::to_byte(uchar* to, const float* from, int W, int delta ) {
    unsigned char* end = to+W*delta;
    int start = rand()%W;
    const float* q;
    unsigned char* p;
    unsigned error;
    /* go fowards from starting point to end of line: */
    error = 0x80;
    for (p = to+start*delta, q = from+start; p < end; p += delta) {
        error = (error&0xff) + to_byte_table[highFloatPart(q)]; ++q;
        *p = (error>>8);
    }
    /* go backwards from starting point to start of line: */
    error = 0x80;
    for (p = to+(start-1)*delta, q = from+start; p >= to; p -= delta) {
        --q; error = (error&0xff) + to_byte_table[highFloatPart(q)];
        *p = (error>>8);
    }
}
void Lut::to_byte(uchar* to, const float* from, const float* alpha, int W, int delta ){
    
    unsigned char* end = to+W*delta;
    int start = rand()%W;
    const float* q;
    const float* a = alpha;
    unsigned char* p;
    unsigned error;
    /* go fowards from starting point to end of line: */
    error = 0x80;
    for (p = to+start*delta, q = from+start,a+=start; p < end; p += delta) {
        const float v = *q * *a;
        error = (error&0xff) + to_byte_table[highFloatPart(&v)]; ++q; ++a;
        *p = (error>>8);
    }
    /* go backwards from starting point to start of line: */
    error = 0x80;
    for (p = to+(start-1)*delta, q = from+start , a = alpha+start; p >= to; p -= delta) {
        const float v = *q * *a;
        --q;--a; error = (error&0xff) + to_byte_table[highFloatPart(&v)];
        *p = (error>>8);
    }

}

void Lut::to_short(U16* to, const float* from, int W, int bits , int delta ){
    (void)to;
    (void)from;
    (void)W;
    (void)bits;
    (void)delta;
    cout << "Lut::to_short not implemented yet." << endl;
}
void Lut::to_short(U16* to, const float* from, const float* alpha, int W, int bits , int delta ){
    (void)to;
    (void)from;
    (void)W;
    (void)bits;
    (void)alpha;
    (void)delta;
    cout << "Lut::to_short not implemented yet." << endl;
}

void Lut::to_float(float* to, const float* from, int W, int delta ){
    for(int i=0;i< W;i+=delta){
        to[i]=toFloatFast(from[i]);
    }
}
void Lut::to_float(float* to, const float* from, const float* alpha, int W, int delta ){
    for(int i=0;i< W;i+=delta){
        to[i]=toFloatFast(from[i])*alpha[i];
    }
}


void Linear::from_byte(float* to, const uchar* from, int W, int delta ){
    from += (W-1)*delta;
    to += W;
    for (; --W >= 0; from -= delta) *--to = *from*(1.0f/255.f);
}
void Linear::from_byteQt(float* to, const QRgb* from,Channel z, int W, int delta ){
    typedef int(*ChannelLookup)(QRgb);
    ChannelLookup lookup;
    switch (z) {
        case Powiter::Channel_alpha:
            lookup = qAlpha;
            break;
        case Powiter::Channel_red:
            lookup = qRed;
            break;
        case Powiter::Channel_green:
            lookup = qGreen;
            break;
        case Powiter::Channel_blue:
            lookup = qBlue;
            break;
        default:
            lookup = qRed;
            break;
    }
    if(z == Powiter::Channel_alpha){
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
    (void)to;
    (void)from;
    (void)W;
    (void)bits;
    (void)delta;
    cout << "Linear::from_short not yet implemented" << endl;
}
void Linear::from_float(float* to, const float* from, int W, int delta ){
    for(int i=0;i< W;i+=delta){
        to[i]=fromFloatFast(from[i]);
    }
}

void Linear::to_byte(uchar* to, const float* from, int W, int delta ){
    unsigned char* end = to+W*delta;
    int start = rand()%W;
    const float* q;
    unsigned char* p;
    /* go fowards from starting point to end of line: */
    float error = .5;
    for (p = to+start*delta, q = from+start; p < end; p += delta) {
        float G = error + *q++ * 255.0f;
        if (G <= 0) {
            *p = 0;
        } else if (G < 255) {
            int i = (int)G;
            *p = i;
            error = G-i;
        } else {
            *p = 255;
        }
    }
    /* go backwards from starting point to start of line: */
    error = .5;
    for (p = to+(start-1)*delta, q = from+start; p >= to; p -= delta) {
        float G = error + *--q * 255.0f;
        if (G <= 0) {
            *p = 0;
        } else if (G < 255) {
            int i = (int)G;
            *p = i;
            error = G-i;
        } else {
            *p = 255;
        }
    }
}

void Linear::to_short(U16* to, const float* from, int W, int bits , int delta ){
    (void)to;
    (void)from;
    (void)W;
    (void)bits;
    (void)delta;
    cout << "Linear::to_short not implemented yet." << endl;
}
void Linear::to_float(float* to, const float* from, int W, int delta ){

    (void)delta;
    memcpy(reinterpret_cast<char*>(to), reinterpret_cast<const char*>(from), W*sizeof(float));
}

Lut* Lut::getLut(DataType type){
    std::map<DataType,Lut*>::iterator found = Lut::_luts.find(type);
    if(found != Lut::_luts.end()){
        return found->second;
    }
    return NULL;
}

Lut* Lut::Linear(){
    Lut* lut=new LinearLut();
    lut->linear(true);
    return lut;
}


void Lut::allocateLuts(){
    Lut* srgb = new sRGB;
    srgb->validate();
    Lut* lin = Lut::Linear();
    lin->validate();
    Lut* rec709 = new Rec709;
    rec709->validate();
    Lut::_luts.insert(make_pair(VIEWER,srgb));
    Lut::_luts.insert(make_pair(FLOAT,lin));
    Lut::_luts.insert(make_pair(INT8,srgb));
    Lut::_luts.insert(make_pair(INT16,srgb));
    Lut::_luts.insert(make_pair(MONITOR,rec709));
}

void Lut::deallocateLuts(){
    for(std::map<DataType,Lut*>::iterator it = _luts.begin();it!=_luts.end();it++){
        if(it->second){
            // now finding in map all other members that have the same pointer to avoid double free
            for(std::map<DataType,Lut*>::iterator it2 = _luts.begin();it2!=_luts.end();it2++){
                if(it2->second == it->second && it->first!=it2->first)
                    it2->second = 0;
            }
            delete it->second;
            it->second = 0;
        }
    }
    _luts.clear();
}
