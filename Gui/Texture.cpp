//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 *Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 *contact: immarespond at gmail dot com
 *
 */

#include "Texture.h"

#include <iostream>
#include "Global/GLIncludes.h"
#include "Gui/ViewerGL.h"

Texture::Texture(){
    glGenTextures(1, &_texID);
    
}
void Texture::fillOrAllocateTexture(const TextureRect& texRect ,DataType type){
    
    
    glEnable(GL_TEXTURE_2D);
    glActiveTexture (GL_TEXTURE0);
    glBindTexture (GL_TEXTURE_2D, _texID);
    if(texRect == _textureRect && _type == type){
        if(_type == Texture::BYTE){
            glTexSubImage2D(GL_TEXTURE_2D,
                            0,				// level
                            0, 0,				// xoffset, yoffset
                            w(), h(),
                            GL_BGRA,			// format
                            GL_UNSIGNED_INT_8_8_8_8_REV,		// type
                            0);
            
            
        }else if(_type == Texture::FLOAT){
            glTexSubImage2D(GL_TEXTURE_2D,
                            0,				// level
                            0, 0 ,				// xoffset, yoffset
                            w(), h(),
                            GL_RGBA,			// format
                            GL_FLOAT,		// type
                            0);
        }
        checkGLErrors();
        
    }else{
        _textureRect = texRect;
        _type = type;
        glPixelStorei (GL_UNPACK_ALIGNMENT, 1);
        
        glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        
        glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
        glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
        if(type == BYTE){
            glTexImage2D(GL_TEXTURE_2D,
                         0,			// level
                         GL_RGBA8, //internalFormat
                         w(), h(),
                         0,			// border
                         GL_BGRA,		// format
                         GL_UNSIGNED_INT_8_8_8_8_REV,	// type
                         0);			// pixels
        }else if(type == FLOAT){
            glTexImage2D (GL_TEXTURE_2D,
                          0,			// level
                          GL_RGBA32F_ARB, //internalFormat
                          w(), h(),
                          0,			// border
                          GL_RGBA,		// format
                          GL_FLOAT,	// type
                          0);			// pixels
        }
        
        checkGLErrors();
    }
}
void Texture::updatePartOfTexture(const TextureRect& fullRegion,int zoomedY,DataType type){
    glEnable(GL_TEXTURE_2D);
    glActiveTexture (GL_TEXTURE0);
    glBindTexture (GL_TEXTURE_2D, _texID);
    if(fullRegion != _textureRect || _type == type){
        _textureRect = fullRegion;
        _type = type;
        glPixelStorei (GL_UNPACK_ALIGNMENT, 1);
        
        glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        
        glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
        glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
        if(type == BYTE){
            glTexImage2D(GL_TEXTURE_2D,
                         0,			// level
                         GL_RGBA8, //internalFormat
                         w(), h(),
                         0,			// border
                         GL_BGRA,		// format
                         GL_UNSIGNED_INT_8_8_8_8_REV,	// type
                         0);			// pixels
        }else if(type == FLOAT){
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
    
    if(_type == Texture::BYTE){
        glTexSubImage2D(GL_TEXTURE_2D,
                        0,				// level
                        0, 0,				// xoffset, yoffset
                        w(), zoomedY+1,
                        GL_BGRA,			// format
                        GL_UNSIGNED_INT_8_8_8_8_REV,		// type
                        0);
        
        
    }else if(_type == Texture::FLOAT){
        glTexSubImage2D(GL_TEXTURE_2D,
                        0,				// level
                        0, 0 ,				// xoffset, yoffset
                        w(), zoomedY+1,
                        GL_RGBA,			// format
                        GL_FLOAT,		// type
                        0);
    }
    checkGLErrors();
    
}

Texture::~Texture(){
    glDeleteTextures(1, &_texID);
}


