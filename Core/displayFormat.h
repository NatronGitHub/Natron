//  Powiter
//
//  Created by Alexandre Gauthier-Foichat on 06/12
//  Copyright (c) 2013 Alexandre Gauthier-Foichat. All rights reserved.
//  contact: immarespond at gmail dot com

#ifndef __DISPLAY_FORMAT_H__
#define __DISPLAY_FORMAT_H__

#include <map>
#include "Core/Box.h"
/*This class is used to hold the format of a frame (its resolution).
 *Some formats have a name , e.g : 1920*1080 is full HD, etc...
 *It also holds a pixel aspect ratio so the viewer can display the
 *frame accordingly*/
class Format:public Box2D{
	
	
public:
	Format(int x,int y,int r,int t,const char* name,double pa=1.0);
    Format(const Format& other):Box2D(other.x(),other.y(),other.right(),other.top()),_name(other.name()),_pixel_aspect(other.pixel_aspect()){}
  
    Format();
    virtual ~Format(){
        //delete [] _name;
    }
    const char* name() const ;
    void name(const char* n){this->_name = n;}
    const double pixel_aspect() const;
    void pixel_aspect( float p) ;
	
	Format operator=(const Format& other){
        set(other.x(), other.y(), other.right(), other.top());
        name(other.name());
        pixel_aspect(other.pixel_aspect());
        return *this;
    }
    bool operator==(const Format& other){
        return _pixel_aspect == other.pixel_aspect() &&
        x() == other.x() &&
        y() == other.y() &&
        right() == other.right() &&
        top() == other.top();
    }
    
private:
	double _pixel_aspect;
    const char* _name;
};


#endif // __DISPLAY_FORMAT_H__