//  Powiter
//
//  Created by Alexandre Gauthier-Foichat on 06/12
//  Copyright (c) 2013 Alexandre Gauthier-Foichat. All rights reserved.
//  contact: immarespond at gmail dot com

#include "Core/displayFormat.h"
using namespace std;


DisplayFormat::DisplayFormat(int x,int y,int r,int t,const char* name,double pa):IntegerBox(x,y,r,t){
    this->_name=name;
    _pixel_aspect=pa;
}

DisplayFormat::DisplayFormat():IntegerBox(),_name(""),_pixel_aspect(0){
    
}

const char* DisplayFormat::name() const {return _name;}

const double DisplayFormat::pixel_aspect() const {return _pixel_aspect;}

void DisplayFormat::pixel_aspect( float p)  {_pixel_aspect=p;}

