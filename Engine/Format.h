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

 

 




#ifndef __DISPLAY_FORMAT_H__
#define __DISPLAY_FORMAT_H__

#include <map>
#include <string>
#include "Engine/Box.h"
/*This class is used to hold the format of a frame (its resolution).
 *Some formats have a name , e.g : 1920*1080 is full HD, etc...
 *It also holds a pixel aspect ratio so the viewer can display the
 *frame accordingly*/
class Format:public Box2D{
	
	
public:
	Format(int x, int y, int r, int t, const std::string& name, double pa=1.0);
    Format(const Format& other):Box2D(other.x(),other.y(),other.right(),other.top()),_pixel_aspect(other.pixel_aspect()),_name(other.name()){}
  
    Format();
    virtual ~Format(){}
    std::string name() const ;
    void name(const std::string& n) const{this->_name = n;}
    double pixel_aspect() const;
    void pixel_aspect( float p) ;
	
	Format operator=(const Format& other){
        set(other.x(), other.y(), other.right(), other.top());
        _name = other.name();
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
    mutable std::string _name;
};


#endif // __DISPLAY_FORMAT_H__
