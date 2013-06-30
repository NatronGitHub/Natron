//  Powiter
//
//  Created by Alexandre Gauthier-Foichat on 06/12
//  Copyright (c) 2013 Alexandre Gauthier-Foichat. All rights reserved.
//  contact: immarespond at gmail dot com

#ifndef __PowiterOsX__texturecache__
#define __PowiterOsX__texturecache__

#include <iostream>
#include "Core/abstractCache.h"
#include "Superviser/powiterFn.h"
/*The texture cache is, as the name says, an object that stores texture previously drawn
 so they can be re-used in the future if we need exactly them. The texture cache is
 useful for panning & zooming.
 Note that a TextureEntry actually doesn't hold any memory, just the adress of the 
 OpenGL texture, so the parameters passed to allocate can be ignored.
 */


class TextureEntry : public CacheEntry{
    
public:
    enum DataType {BYTE = 0,FLOAT = 1};

protected:
    U32 _texID;
    int _w,_h;
    DataType _type;
public:
    
    
    TextureEntry();
    
    U32 getTexID() const {return _texID;}
    
    int w() const {return _w;}
    
    int h() const {return _h;}
    
    DataType type() const {return _type;}
    
    /*allocates the texture*/
    void allocate(int w, int h ,DataType type);
    
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
    
    ~TextureCache(){}
    
    virtual  std::string cacheName() {return "TextureCache";}
    
    /*Returns 0 if the texture represented by the key is not present in the cache.*/
    TextureEntry* get(U64 key);
    
    /*Inserts a new texture,represented by key in the cache.*/
    TextureEntry* addTexture(U64 key,int w , int h ,TextureEntry::DataType type);
    
};

#endif /* defined(__PowiterOsX__texturecache__) */
