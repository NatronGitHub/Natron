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

 

 



#include "Engine/Box.h"
#include <iostream>

void Box2D::merge(const Box2D& b){
	int x=b.x();
	int y=b.y();
	int r=b.right();
	int t=b.top();

	if(x<this->x())
		_x=x;
	if(y<this->y())
		_y=y;
	if(r>this->right())
		_r=r;
	if(t>this->top())
		_t=t;
    _modified = true;
}

void Box2D::merge(int x, int y){
	if(x<this->x())
		_x=x;
	if(y<this->y())
		_y=y;
    _modified = true;
}

void Box2D::merge(int x, int y, int r, int t){
	if(x<this->x())
		_x=x;
	if(y<this->y())
		_y=y;
	if(r>this->right())
		_r=r;
	if(t>this->top())
		_t=t;
    _modified = true;
}

void Box2D::intersect(const Box2D b){
	int x=b.x();
	int y=b.y();
	int t=b.top();
	int r=b.right();

	if(x>_x)
		_x=x;
	if(y>_y)
		_y=y;
	if(r<_r)
		_r=r;
	if(t<_t)
		_t=t;
}



void Box2D::intersect(int x, int y, int r, int t){
	if(x>_x)
		_x=x;
	if(y>_y)
		_y=y;
	if(r<_r)
		_r=r;
	if(t<_t)
		_t=t;
}

bool Box2D::isContained(int x,int y,int r,int t){
	bool stillgood=true;
	if(x<this->x())
		stillgood=false;
	if(y<this->y())
		stillgood=false;
	if(r>this->right())
		stillgood=false;
	if(t>this->top())
		stillgood=false;
	return stillgood;

}
bool Box2D::isContained(const Box2D b){

	int x=b.x();
	int y=b.y();
	int t=b.top();
	int r=b.right();
	bool stillgood=true;
	if(x<this->x())
		stillgood=false;
	if(y<this->y())
		stillgood=false;
	if(r>this->right())
		stillgood=false;
	if(t>this->top())
		stillgood=false;
	return stillgood;
}
