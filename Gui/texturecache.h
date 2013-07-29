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
#include "Core/abstractCache.h"
#include "Core/Box.h"
#include "Superviser/powiterFn.h"

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
    
    int x,y,r,t;
    int w,h;
};

/** @class The texture cache is, as the name says, an object that stores texture previously drawn
 so they can be re-used in the future if we need exactly them. The texture cache is
 useful for panning & zooming.
 Note that a TextureEntry actually doesn't hold any memory, just the adress of the 
 OpenGL texture, so the parameters passed to allocate can be ignored.
 **/
class TextureEntry : public CacheEntry{
public:
    /*Note that the half datatype is not used currently*/
    enum DataType {BYTE = 0,FLOAT = 1 , HALF = 2};

protected:
    U64 _hashKey;
    U32 _texID;
    TextureRect _textureRect;
    DataType _type;
public:
    
    
    TextureEntry();
    
    U32 getTexID() const {return _texID;}
    
    int w() const {return _textureRect.w;}
    
    int h() const {return _textureRect.h;}
    
    DataType type() const {return _type;}
    
    /*allocates the texture*/
    void allocate(const TextureRect& texRect,DataType type);
    
    void setHashKey(U64 key){_hashKey = key;}
        
    const TextureRect& getTextureRect() const {return _textureRect;}
    
    U64 getHashKey() const {return _hashKey;}
    
    /*deallocates the texture*/
    virtual void deallocate();
    
    virtual ~TextureEntry();
    
private:
    
    /*private hack : we don't use this function here*/
    virtual bool allocate(U64 ,const char* path = 0){(void)path;return true;}
    
};

class TextureCache : public AbstractCache{
    
public:
       
    TextureCache():AbstractCache(){}
    
    virtual ~TextureCache(){}
    
    virtual  std::string cacheName() {return "TextureCache";}
    
    /*Returns 0 if the texture represented by the key is not present in the cache.*/
    TextureEntry* get(U64 key);
    
    /*Inserts an existing texture in the cache*/
    void addTexture(TextureEntry* texture);
    
    void remove(TextureEntry* entry);
    
};

#endif /* defined(__PowiterOsX__texturecache__) */
