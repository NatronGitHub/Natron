//  Powiter
//
//  Created by Alexandre Gauthier-Foichat on 06/12
//  Copyright (c) 2013 Alexandre Gauthier-Foichat. All rights reserved.
//  contact: immarespond at gmail dot com
#include "Superviser/gl_OsDependent.h"
#include "texturecache.h"

using namespace std;

TextureCache::TextureKey::TextureKey():_exposure(0),_lut(0),_zoomFactor(0),_w(0),_h(0),_byteMode(0),
_treeVersion(0),_firstRow(0),_lastRow(0){}

TextureCache::TextureKey::TextureKey(const TextureKey& other):_exposure(other._exposure),
_lut(other._lut),_zoomFactor(other._zoomFactor),_w(other._w),_h(other._h),_byteMode(other._byteMode),
_filename(other._filename),_treeVersion(other._treeVersion),_firstRow(other._firstRow),_lastRow(other._lastRow){}

TextureCache::TextureKey::TextureKey(float exp,float lut,float zoom,int w,int h,float bytemode,
                                     std::string filename,U64 treeVersion,int firstRow,int lastRow):
_exposure(exp),_lut(lut),_zoomFactor(zoom),_w(w),_h(h),_byteMode(bytemode),_filename(filename),_treeVersion(treeVersion),
_firstRow(firstRow),_lastRow(lastRow){}

bool TextureCache::TextureKey::operator==(const TextureKey& other){
    /*comparison is done by by ordering members such that the ones
     that are the most likely to be different are compared first*/
    if(_byteMode == 1.0){ // we care of the lut since it was applied by software
        return _exposure ==_firstRow == other._firstRow &&
        _lastRow == other._lastRow &&
        _zoomFactor == other._zoomFactor &&
        other._exposure &&
        _lut == other._lut &&
        _w == other._w &&
        _h == other._h &&
        _byteMode == other._byteMode &&
        _filename == other._filename &&
        _treeVersion == other._treeVersion;
        
    }else{ // don't care of the lut and exposure
        return _exposure == _firstRow == other._firstRow &&
        _lastRow == other._lastRow &&
        _zoomFactor == other._zoomFactor &&
        _w == other._w &&
        _h == other._h &&
        _byteMode == other._byteMode &&
        _filename == other._filename &&
        _treeVersion == other._treeVersion ;
        
    }
}
void TextureCache::TextureKey::operator=(const TextureKey& other){
    _exposure = other._exposure;
    _lut = other._lut;
    _zoomFactor = other._zoomFactor;
    _w = other._w;
    _h = other._h;
    _byteMode = other._byteMode;
    _filename = other._filename;
    _treeVersion = other._treeVersion;
    _firstRow = other._firstRow;
    _lastRow = other._lastRow;
}


/*Returns true if the texture represented by the key is present in the cache.*/
TextureCache::TextureIterator TextureCache::isCached(TextureCache::TextureKey &key){
    for(TextureIterator it = _cache.begin() ; it != _cache.end() ; it++){
        if(it->first == key){
            return it;
        }
    }
    return _cache.end();
}
/*Frees approximatively 10% of the texture cache*/
void TextureCache::makeFreeSpace(){
    size_t sizeTargeted = (size_t)((float)_size*9.f/10.f);
    while (_size > sizeTargeted) {
        pair<TextureKey,U32>& tex = _cache.back();
        size_t dataSize = 0;
        if(tex.first._byteMode == 1.f){
            dataSize = tex.first._w * tex.first._h * sizeof(U32);
        }else{
            dataSize = tex.first._w * tex.first._h * 4 * sizeof(float);
        }
        _size -= dataSize;
        glDeleteTextures(1, &tex.second);
        _cache.pop_back();
    }
}

/*Inserts a new texture,represented by key in the cache. This function must be called
 only if  isCached(...) returned false with this key*/
void TextureCache::append(TextureCache::TextureKey key){
    size_t dataSize = 0;
    if(key._byteMode == 1.f){
        dataSize = key._w * key._h * sizeof(U32);
    }else{
        dataSize = key._w * key._h * 4 * sizeof(float);
    }
    if(_size + dataSize > _maxSizeAllowed){
        makeFreeSpace();
    }
    U32 texID = initializeTexture(key);
    _cache.push_back(make_pair(key,texID));
}


U32 initializeTexture(TextureCache::TextureKey& key){
    GLuint texID;
    glEnable(GL_TEXTURE_2D);
    glGenTextures(1, &texID);
    glPixelStorei (GL_UNPACK_ALIGNMENT, 1);
    glBindTexture (GL_TEXTURE_2D, texID);
    if(key._zoomFactor >= 0.5){
        glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    }else{
        glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    if(key._byteMode==1){
        glTexImage2D (GL_TEXTURE_2D,
                      0,			// level
                      GL_RGBA8, //internalFormat
                      key._w, key._h,
                      0,			// border
                      GL_BGRA,		// format
                      GL_UNSIGNED_INT_8_8_8_8_REV,	// type
                      0);			// pixels
    }else{
        glTexImage2D (GL_TEXTURE_2D,
                      0,			// level
                      GL_RGBA32F_ARB, //internalFormat
                      key._w, key._h,
                      0,			// border
                      GL_RGBA,		// format
                      GL_FLOAT,	// type
                      0);			// pixels
    }
    glBindTexture(GL_TEXTURE_2D, 0);
    return (U32)texID;
}
