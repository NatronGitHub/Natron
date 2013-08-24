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

 

 




#ifndef __PowiterOsX__texturecache__
#define __PowiterOsX__texturecache__

#include <iostream>
#include "Engine/Box.h"

/** @class This class describes the rectangle (or portion) of an image that is contained
 *into a texture. x,y,r,t are respectivly the image coordinates of the left,bottom,right,top
 *edges of the texture. w,h are the width and height of the texture. Note that r - x != w
 *and likewise t - y != h , this is because a texture might not contain all the lines/columns
 *of the image in the portion defined by x,y,r,t.
 **/
class TextureRect{
public:
    
    TextureRect() : x(0) , y(0) , r(0) , t(0) , w(0) , h(0) {}
    
    TextureRect(int x,int y,int r,int t,int w,int h) :
    x(x),
    y(y),
    r(r),
    t(t),
    w(w),
    h(h){}
    
    int x,y,r,t; // the edges of the texture. These are coordinates in the full size image
    int w,h; // the width and height of the texture. This has nothing to do with x,y,r,t
};
inline bool operator==(const TextureRect& first ,const TextureRect& second){
   return first.x == second.x &&
    first.y == second.y &&
    first.r == second.r &&
    first.t == second.t &&
    first.w == second.w &&
    first.h == second.h;
}

inline bool operator!=(const TextureRect& first ,const TextureRect& second){ return !(first == second); }


class Texture{
public:
    /*Note that the short datatype is not used currently*/
    enum DataType {BYTE = 0,FLOAT = 1 , SHORT = 2};

private:
    U32 _texID;
    TextureRect _textureRect;
    DataType _type;
public:
    
    
    Texture();
    
    U32 getTexID() const {return _texID;}
    
    int w() const {return _textureRect.w;}
    
    int h() const {return _textureRect.h;}
    
    DataType type() const {return _type;}
    
    /*allocates the texture*/
    void fillOrAllocateTexture(const TextureRect& texRect,DataType type);
    
    void updatePartOfTexture(const TextureRect& fullRegion,int zoomedY,DataType type);
            
    const TextureRect& getTextureRect() const {return _textureRect;}
   
    
    virtual ~Texture();
    
private:
    
    /*private hack : we don't use this function here*/
    virtual bool allocate(U64 ,const char* path = 0){(void)path;return true;}
    
};



#endif /* defined(__PowiterOsX__texturecache__) */
