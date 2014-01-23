//  Natron
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
*Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012. 
*contact: immarespond at gmail dot com
*
*/

#ifndef NATRON_GUI_TEXTURE_H_
#define NATRON_GUI_TEXTURE_H_

#include "Global/Macros.h"
#include "Global/GlobalDefines.h"
#include "Engine/Rect.h"


class Texture{
public:
    /*Note that the short datatype is not used currently*/
    enum DataType {BYTE = 0,FLOAT = 1 , HALF_FLOAT = 2};

public:
    
    
    Texture();
    
    U32 getTexID() const {return _texID;}
    
    int w() const {return _textureRect.x2 - _textureRect.x1;}
    
    int h() const {return _textureRect.y2 - _textureRect.y1;}
    
    DataType type() const {return _type;}
    
    /*allocates the texture*/
    void fillOrAllocateTexture(const RectI& texRect,DataType type);
    
    void updatePartOfTexture(const RectI& fullRegion,int zoomedY,DataType type);
            
    const RectI& getTextureRect() const {return _textureRect;}
   
    
    virtual ~Texture();
    
private:
    
    /*private hack : we don't use this function here*/
    virtual bool allocate(U64 ,const char* path = 0){(void)path;return true;}

private:
    U32 _texID;
    RectI _textureRect;
    DataType _type;
};



#endif /* defined(NATRON_GUI_TEXTURE_H_) */
