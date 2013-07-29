//  Powiter
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
*Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012. 
*contact: immarespond at gmail dot com
*
*/

 

 



#include "Superviser/gl_OsDependent.h"
#include "Gui/GLViewer.h"
#include "Gui/texturecache.h"

using namespace std;

TextureEntry::TextureEntry():CacheEntry(),_hashKey(0){
    glGenTextures(1, &_texID);

}
void TextureEntry::allocate(const TextureRect& texRect ,DataType type){
    if(_textureRect.w == texRect.w && _textureRect.h == texRect.h && _type == type)
        return;
    _textureRect = texRect;
    _type = type;
    glPixelStorei (GL_UNPACK_ALIGNMENT, 1);
    glActiveTexture (GL_TEXTURE0);
    glEnable(GL_TEXTURE_2D);
    glBindTexture (GL_TEXTURE_2D, _texID);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,GL_CLAMP);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    if(type == BYTE){
        _size = w()*h()*sizeof(U32);
        glTexImage2D(GL_TEXTURE_2D,
                      0,			// level
                      GL_RGBA8, //internalFormat
                      w(), h(),
                      0,			// border
                      GL_BGRA,		// format
                      GL_UNSIGNED_INT_8_8_8_8_REV,	// type
                      0);			// pixels
    }else if(type == FLOAT){
        _size = w()*h()*4*sizeof(float);
        glTexImage2D (GL_TEXTURE_2D,
                      0,			// level
                      GL_RGBA32F_ARB, //internalFormat
                      w(), h(),
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
void TextureCache::addTexture(TextureEntry* texture){
    AbstractCache::add(texture->getHashKey(),texture);
}



