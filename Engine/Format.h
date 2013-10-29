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

#ifndef POWITER_ENGINE_FORMAT_H_
#define POWITER_ENGINE_FORMAT_H_

#include <string>
#include <QtCore/QMetaType>
#include "Engine/Box.h"
#include <boost/serialization/base_object.hpp>
#include <boost/serialization/export.hpp>
/*This class is used to hold the format of a frame (its resolution).
 *Some formats have a name , e.g : 1920*1080 is full HD, etc...
 *It also holds a pixel aspect ratio so the viewer can display the
 *frame accordingly*/
class Format : public Box2D {
	
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version)
    {
        (void)version;
        boost::serialization::void_cast_register<Format,Box2D>(
                                                                static_cast<Format *>(NULL),
                                                                static_cast<Box2D *>(NULL)
                                                               );
        ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Box2D);
        ar & boost::serialization::make_nvp("Pixel_aspect_ratio",_pixel_aspect);
        ar & boost::serialization::make_nvp("Name",_name);
        
    }
	
public:
    Format(int l, int b, int r, int t, const std::string& name, double pa)
    : Box2D(l,b,r,t)
    , _pixel_aspect(pa)
    , _name(name)
    {}
    
    Format(const Format& other)
    :Box2D(other.left(),other.bottom(),other.right(),other.top())
    ,_pixel_aspect(other.getPixelAspect())
    ,_name(other.getName())
    {}
    
    Format()
    : Box2D()
    , _pixel_aspect(1.0)
    , _name()
    {
    }
    
    virtual ~Format(){}
    
    const std::string& getName() const {return _name;}
    
    void setName(const std::string& n) {_name = n;}
    
    double getPixelAspect() const {return _pixel_aspect;}

    void setPixelAspect( double p) {_pixel_aspect = p;}
	
	Format operator=(const Format& other){
        set(other.left(), other.bottom(), other.right(), other.top());
        setName(other.getName());
        setPixelAspect(other.getPixelAspect());
        return *this;
    }
    bool operator==(const Format& other) const {
        return _pixel_aspect == other.getPixelAspect() &&
        left() == other.left() &&
        bottom() == other.bottom() &&
        right() == other.right() &&
        top() == other.top();
    }
    
  
    
private:
	double _pixel_aspect;
    std::string _name;
    
};

Q_DECLARE_METATYPE(Format);


#endif // POWITER_ENGINE_FORMAT_H_
