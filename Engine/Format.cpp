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

 

 




#include "Engine/Format.h"
using namespace std;


Format::Format(int x, int y, int r, int t, const std::string& name, double pa)
: Box2D(x,y,r,t)
, _pixel_aspect(pa)
, _name(name)
{
}

Format::Format()
: Box2D()
, _pixel_aspect(0)
, _name("")
{
}

std::string Format::name() const {return _name;}

double Format::pixel_aspect() const {return _pixel_aspect;}

void Format::pixel_aspect( float p)  {_pixel_aspect=p;}

