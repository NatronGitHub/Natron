//  Powiter
//
//  Created by Alexandre Gauthier-Foichat on 06/12
//  Copyright (c) 2013 Alexandre Gauthier-Foichat. All rights reserved.
//  contact: immarespond at gmail dot com
#include "Superviser/gl_OsDependent.h"
#include "Gui/texturecache.h"

using namespace std;

TextureEntry::TextureEntry():CacheEntry(),_w(0),_h(0){
    glGenTextures(1, &_texID);

}
void TextureEntry::allocate(int w, int h ,DataType type){
    if(_w == w && _h == h && _type == type) return;
    _w = w;
    _h = h;
    _type = type;
    glPixelStorei (GL_UNPACK_ALIGNMENT, 1);
    glActiveTexture (GL_TEXTURE0);
    glBindTexture (GL_TEXTURE_2D, _texID);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    if(type == BYTE){
        _size = w*h*sizeof(U32);
        glTexImage2D (GL_TEXTURE_2D,
                      0,			// level
                      GL_RGBA8, //internalFormat
                      w, h,
                      0,			// border
                      GL_BGRA,		// format
                      GL_UNSIGNED_INT_8_8_8_8_REV,	// type
                      0);			// pixels
    }else if(type == FLOAT){
        _size = w*h*4*sizeof(float);
        glTexImage2D (GL_TEXTURE_2D,
                      0,			// level
                      GL_RGBA32F_ARB, //internalFormat
                      w, h,
                      0,			// border
                      GL_RGBA,		// format
                      GL_FLOAT,	// type
                      0);			// pixels
    }

}
void TextureEntry::deallocate(){
    glDeleteTextures(1, &_texID);
}
TextureEntry::~TextureEntry(){
    deallocate();
}


/*Returns true if the texture represented by the key is present in the cache.*/
TextureEntry* TextureCache::get(U64 key){
    CacheIterator it = isCached(key);
    if(it != end()){
        TextureEntry* texEntry = dynamic_cast<TextureEntry*>(it->second);
        assert(texEntry);
        return texEntry;
    }
    return NULL;
}

/*Inserts a new texture,represented by key in the cache. This function must be called
 only if  isCached(...) returned false with this key*/
TextureEntry* TextureCache::addTexture(U64 key,int w , int h ,TextureEntry::DataType type){
    TextureEntry* entry = new TextureEntry;
    entry->allocate(w,h,type);
    AbstractCache::add(key,entry);
    return entry;
}




