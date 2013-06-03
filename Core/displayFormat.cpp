//  Powiter
//
//  Created by Alexandre Gauthier-Foichat on 06/12
//  Copyright (c) 2013 Alexandre Gauthier-Foichat. All rights reserved.
//  contact: immarespond at gmail dot com

#include "Core/displayFormat.h"
using namespace std;


Format::Format(int x,int y,int r,int t,std::string name,double pa):Box2D(x,y,r,t){
    this->_name=name;
    _pixel_aspect=pa;
}

Format::Format():Box2D(),_name(""),_pixel_aspect(0){
    
}

std::string Format::name() const {return _name;}

const double Format::pixel_aspect() const {return _pixel_aspect;}

void Format::pixel_aspect( float p)  {_pixel_aspect=p;}

