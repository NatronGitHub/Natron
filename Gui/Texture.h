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

#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>

#include "Global/Macros.h"
#include "Global/GlobalDefines.h"
#include "Engine/TextureRect.h"



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

namespace boost {
    namespace serialization {
        
        template<class Archive>
        void serialize(Archive & ar, TextureRect &t, const unsigned int version)
        {
            (void)version;
            ar & t.x & t.y & t.r & t.t & t.w & t.h;
        }
    }
}



#endif /* defined(NATRON_GUI_TEXTURE_H_) */
