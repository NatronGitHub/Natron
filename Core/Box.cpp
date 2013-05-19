//  Powiter
//
//  Created by Alexandre Gauthier-Foichat on 06/12
//  Copyright (c) 2013 Alexandre Gauthier-Foichat. All rights reserved.
//  contact: immarespond at gmail dot com
#include "Core/Box.h"
#include <iostream>

void IntegerBox::merge(const IntegerBox b){
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
}

void IntegerBox::merge(int x, int y){
	if(x<this->x())
		_x=x;
	if(y<this->y())
		_y=y;
}

void IntegerBox::merge(int x, int y, int r, int t){
	if(x<this->x())
		_x=x;
	if(y<this->y())
		_y=y;
	if(r>this->right())
		_r=r;
	if(t>this->top())
		_t=t;
}

void IntegerBox::intersect(const IntegerBox b){
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



void IntegerBox::intersect(int x, int y, int r, int t){
	if(x>_x)
		_x=x;
	if(y>_y)
		_y=y;
	if(r<_r)
		_r=r;
	if(t<_t)
		_t=t;
}

bool IntegerBox::isContained(int x,int y,int r,int t){
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
bool IntegerBox::isContained(const IntegerBox b){

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
