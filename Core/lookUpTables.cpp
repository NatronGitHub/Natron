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
	return lookup_fromByteLUT(v);
}

/*Converts a single floating point value from linear to the LUT space by using the lookup tables. */
float Lut::toFloatFast(float v) {
    return lookup_toByteLUT(v)/255.f;
}


void Lut::from_byte(float* r,float* g,float* b, const uchar* from,bool hasAlpha,bool premult,bool autoAlpha, int W, int delta ,float* a,bool qtbuf){
    const QRgb* ptrQt = reinterpret_cast<const QRgb*>(from);
    
    if(hasAlpha && premult){//if img is premultiplied,un-premultiplied before applying color space conversion, and then multiply back
        //alpha channel is copied through Linear (no conversion)
        for(int i=0;i< W;i++){
            float red,green,blue,alphac=1.f;
            if(qtbuf){
                const QRgb c = ptrQt[i];
                red = qRed(c);
                green = qGreen(c);
                blue = qBlue(c);
                alphac = qAlpha(c);
            }else{
                const uchar *c = from+i*3;
                Lut::U24_to_rgb(&red, &green, &blue, c);
               
            }
            if(r!=NULL)
                r[i]=from_byte_table[(int)(red/alphac)] * alphac;
            if(g!=NULL)
                g[i]=from_byte_table[(int)(green/alphac)] * alphac;
            if(b!=NULL)
                b[i]=from_byte_table[(int)(blue/alphac)] * alphac;
            a[i]=Linear::fromFloatFast((float)alphac/255.f);
        }
        
    }else if(hasAlpha && !premult){
        for(int i=0;i< W;i++){
            float red,green,blue,alphac=1.f;
            if(qtbuf){
                const QRgb c = ptrQt[i];
                red = qRed(c);
                green = qGreen(c);
                blue = qBlue(c);
                alphac = qAlpha(c);
            }else{
                const uchar *c = from+i*3;
                Lut::U24_to_rgb(&red, &green, &blue, c);
               
            }
            if(r!=NULL)
                r[i]=from_byte_table[(int)red];
            if(g!=NULL)
                g[i]=from_byte_table[(int)green];
            if(b!=NULL)
                b[i]=from_byte_table[(int)blue];
            a[i]=Linear::fromFloatFast((float)alphac/255.f);
        }
        
    }else if(!hasAlpha && autoAlpha){
        for(int i=0;i< W;i++){
            float red,green,blue,alphac=1.f;
            if(qtbuf){
                const QRgb c = ptrQt[i];
                red = qRed(c);
                green = qGreen(c);
                blue = qBlue(c);
                alphac = 1.0;
            }else{
                const uchar *c = from+i*3;
                Lut::U24_to_rgb(&red, &green, &blue, c);
             
            }
            if(r!=NULL)
                r[i]=from_byte_table[(int)red];
            if(g!=NULL)
                g[i]=from_byte_table[(int)green];
            if(b!=NULL)
                b[i]=from_byte_table[(int)blue];
            a[i]=Linear::fromFloatFast(alphac);
        }
        
    }else if(!hasAlpha && !autoAlpha){
        for(int i=0;i< W;i++){
            float red,green,blue;
            if(qtbuf){
                const QRgb c = ptrQt[i];
                red = qRed(c);
                green = qGreen(c);
                blue = qBlue(c);
                
            }else{
                const uchar *c = from+i*3;
                Lut::U24_to_rgb(&red, &green, &blue, c);
                
            }
            if(r!=NULL)
                r[i]=fromFloatFast(red);
            if(g!=NULL)
                g[i]=fromFloatFast(green);
            if(b!=NULL)
                b[i]=fromFloatFast(blue);
            
        }
    }
}

void Lut::from_short(float* r,float* g,float* b, const U16* from, const U16* alpha,bool premult,bool autoAlpha, int W, int bits, int delta ,float* a){
    cout << "Lut::from not yet implemented" << endl;
    
}
void Lut::from_float(float* r,float* g,float* b, const float* fromR,const float* fromG,const float* fromB,
                bool premult,bool autoAlpha, int W, int delta ,const float* fromA,float* a){
    
    if(fromA!=NULL && premult){//if img is premultiplied,un-premultiplied before applying color space conversion, and then multiply back
        //alpha channel is copied through Linear (no conversion)
        for(int i=0;i< W;i++){
            float red = fromR[i];
            float green = fromG[i];
            float blue = fromB[i];
            float alphac = fromA[i];
            if(r!=NULL)
                r[i]=fromFloatFast((red/alphac)*255.f) * alphac*255.f;
            if(g!=NULL)
                g[i]=fromFloatFast((green/alphac)*255.f) * alphac*255.f;
            if(b!=NULL)
                b[i]=fromFloatFast((blue/alphac)*255.f) * alphac*255.f;
            a[i]=Linear::fromFloat(alphac*255.f);
        }
        
    }else if(fromA!=NULL && !premult){
        for(int i=0;i< W;i++){
            float red = fromR[i];
            float green = fromG[i];
            float blue = fromB[i];
            float alphac = fromA[i];
            if(r!=NULL)
                r[i]=fromFloatFast(red*255.f);
            if(g!=NULL)
                g[i]=fromFloatFast(green*255.f);
            if(b!=NULL)
                b[i]=fromFloatFast(blue*255.f);
            a[i]=Linear::fromFloat(alphac*255.f);
        }
        
    }else if(fromA!=NULL && autoAlpha){
        for(int i=0;i< W;i++){
            float red = fromR[i];
            float green = fromG[i];
            float blue = fromB[i];
            float alphac = 1.0;
            if(r!=NULL)
                r[i]=fromFloatFast(red*255.f);
            if(g!=NULL)
                g[i]=fromFloatFast(green*255.f);
            if(b!=NULL)
                b[i]=fromFloatFast(blue*255.f);
            a[i]=Linear::fromFloat(alphac*255.f);
        }
        
    }else if(fromA==NULL && !autoAlpha){
        for(int i=0;i< W;i++){
            float red = fromR[i];
            float green = fromG[i];
            float blue = fromB[i];
            if(r!=NULL)
                r[i]=fromFloatFast(red*255.f);
            if(g!=NULL)
                g[i]=fromFloatFast(green*255.f);
            if(b!=NULL)
                b[i]=fromFloatFast(blue*255.f);
        }
    }
    
    
    
}
    


void Linear::from_byte(float* r,float* g,float* b, const uchar* from,bool hasAlpha,bool premult,bool autoAlpha, int W, int delta ,float* a,bool qtbuf){
    
	const QRgb* ptrQt = reinterpret_cast<const QRgb*>(from);

	if(hasAlpha && premult){//if img is premultiplied,un-premultiplied before applying color space conversion, and then multiply back
		//alpha channel is copied through Linear (no conversion)
		for(int i=0;i< W;i++){
			float red,green,blue,alphac=1;
			if(qtbuf){
				const QRgb c = ptrQt[i];
                red = qRed(c);
                green = qGreen(c);
                blue = qBlue(c);
                alphac = qAlpha(c);
			}else{
				const uchar *c = from+i*3;
				Lut::U24_to_rgb(&red, &green, &blue, c);

			}
            red/=255.f;
            green/=255.f;
            blue/=255.f;
            alphac/=255.f;
            if(r!=NULL)
                r[i]=Linear::fromFloatFast(red/alphac) * alphac;
            if(g!=NULL)
                g[i]=Linear::fromFloatFast(green/alphac) * alphac;
            if(b!=NULL)
                b[i]=Linear::fromFloatFast(blue/alphac) * alphac;
			a[i]=Linear::fromFloatFast(alphac);
		}

	}else if(hasAlpha && !premult){
		for(int i=0;i< W;i++){
			float red,green,blue,alphac=1;
			if(qtbuf){
				const QRgb c = ptrQt[i];
                red = qRed(c);
                green = qGreen(c);
                blue = qBlue(c);
                alphac = qAlpha(c);
			}else{
				const uchar *c = from+i*3;
				Lut::U24_to_rgb(&red, &green, &blue, c);

			}
            red/=255.f;
            green/=255.f;
            blue/=255.f;
            alphac/=255.f;
            if(r!=NULL)
                r[i]=Linear::fromFloatFast(red);
            if(g!=NULL)
                g[i]=Linear::fromFloatFast(green);
            if(b!=NULL)
                b[i]=Linear::fromFloatFast(blue);
			a[i]=Linear::fromFloatFast(alphac);
		}

	}else if(!hasAlpha && autoAlpha){
		for(int i=0;i< W;i++){
			float red,green,blue,alphac=1;
			if(qtbuf){
				const QRgb c = ptrQt[i];
                red = qRed(c);
                green = qGreen(c);
                blue = qBlue(c);
				alphac = 1.0;
			}else{
				const uchar *c = from+i*3;
				Lut::U24_to_rgb(&red, &green, &blue, c);

			}
            red/=255.f;
            green/=255.f;
            blue/=255.f;
            alphac/=255.f;
			if(r!=NULL)
                r[i]=Linear::fromFloatFast(red);
            if(g!=NULL)
                g[i]=Linear::fromFloatFast(green);
            if(b!=NULL)
                b[i]=Linear::fromFloatFast(blue);
			a[i]=Linear::fromFloatFast(alphac);
		}

	}else if(!hasAlpha && !autoAlpha){
		for(int i=0;i< W;i++){
			float red,green,blue;
			if(qtbuf){
				const QRgb c = ptrQt[i];
                red = qRed(c);
                green = qGreen(c);
                blue = qBlue(c);
           

			}else{
				const uchar *c = from+i*3;
				Lut::U24_to_rgb(&red, &green, &blue, c);

			}
            red/=255.f;
            green/=255.f;
            blue/=255.f;
            
			if(r!=NULL)
                r[i]=Linear::fromFloatFast(red);
            if(g!=NULL)
                g[i]=Linear::fromFloatFast(green);
            if(b!=NULL)
                b[i]=Linear::fromFloatFast(blue);

		}
	}

}

void Linear::from_short(float* r,float* g,float* b, const U16* from, const U16* alpha,bool premult, bool autoAlpha,int W, int bits, int delta ,float* a){
    cout << "Linear::from not yet implemented" << endl;
    
}
void Linear::from_float(float* r,float* g,float* b, const float* fromR,const float* fromG,const float* fromB,
                     bool premult,bool autoAlpha, int W, int delta ,const float* fromA,float* a){
    
    // we don't have to copy anything : no colorspace conversion is done & data is already present in the buffer
    // if autoalpha is on , we just fill the alpha channel with 1.f
    if(fromA!=NULL && autoAlpha){
        for(int i=0;i< W;i++){
            a[i]=1.0;
        }
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