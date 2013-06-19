//  Powiter
//
//  Created by Alexandre Gauthier-Foichat on 06/12
//  Copyright (c) 2013 Alexandre Gauthier-Foichat. All rights reserved.
//  contact: immarespond at gmail dot com

#ifndef __PowiterOsX__texturecache__
#define __PowiterOsX__texturecache__

#include <iostream>
#include <vector>
#include <utility>
#include "Superviser/powiterFn.h"
/*The texture cache is, as the name says, an object that stores texture previously drawn
 so they can be re-used in the future if we need exactly them. The texture cache is
 useful for panning & zooming. Hence the data structure used here is not the same
 as the one used for the viewerCache (i.e: a multimap with filename as a key).
 This is because all the textures will most likely come from a same filename if the 
 user panned around.
 Freeing policy is : most recently used*/
class TextureCache{
    
public:
    /*This class is a way to uniquely identify textures*/
    class TextureKey{
    public:
        float _exposure;
        float _lut;
        float _zoomFactor;
        int _w,_h;
        float _byteMode;
        std::string _filename;
        U64 _treeVersion;
        int _firstRow,_lastRow;
        
        TextureKey();
        TextureKey(const TextureKey& other);
        TextureKey(float exp,float lut,float zoom,int w,int h,float bytemode,
                   std::string filename,U64 treeVersion,int firstRow,int lastRow);
        bool operator==(const TextureKey& other);
        void operator=(const TextureKey& other);
        
    };
    
    TextureCache(U64 maxSize):_maxSizeAllowed(maxSize),_size(0){}
    ~TextureCache(){}
    
    typedef std::vector< std::pair<TextureKey,U32> >::iterator TextureIterator;
    typedef std::vector< std::pair<TextureKey,U32> >::reverse_iterator TextureReverseIterator;
    
    /*Returns true if the texture represented by the key is present in the cache.*/
    TextureCache::TextureIterator isCached(TextureCache::TextureKey &key);
    
    TextureCache::TextureIterator end(){return _cache.end();}
    
    /*Frees approximatively 10% of the texture cache*/
    void makeFreeSpace();
    
    /*Inserts a new texture,represented by key in the cache. This function must be called
     only if  isCached(...) returned false with this key*/
    U32 append(TextureCache::TextureKey key);
    
    /*clears out the texture cache*/
    void clearCache(const std::vector<U32>& currentlyDisplayedTex);
    
    /*Returns the current size of the cache*/
    U64 size(){return _size;}
    
private:
    /*Initialize an OpenGL texture and returns the texture ID*/
    U32 initializeTexture();
    
    
    std::vector< std::pair<TextureKey,U32> > _cache; // key, texture Id
    U64 _size;
    U64 _maxSizeAllowed;
};

#endif /* defined(__PowiterOsX__texturecache__) */
