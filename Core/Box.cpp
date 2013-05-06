#include "Core/Box.h"
#include <iostream>

void IntegerBox::merge(const IntegerBox b){
	int x=b.x();
	int y=b.y();
	int r=b.range();
	int t=b.top();

	if(x<this->x())
		x_=x;
	if(y<this->y())
		y_=y;
	if(r>this->range())
		r_=r;
	if(t>this->top())
		t_=t;
}

void IntegerBox::merge(int x, int y){
	if(x<this->x())
		x_=x;
	if(y<this->y())
		y_=y;
}

void IntegerBox::merge(int x, int y, int r, int t){
	if(x<this->x())
		x_=x;
	if(y<this->y())
		y_=y;
	if(r>this->range())
		r_=r;
	if(t>this->top())
		t_=t;
}

void IntegerBox::intersect(const IntegerBox b){
	int x=b.x();
	int y=b.y();
	int t=b.top();
	int r=b.range();

	if(x>x_)
		x_=x;
	if(y>y_)
		y_=y;
	if(r<r_)
		r_=r;
	if(t<t_)
		t_=t;
}



void IntegerBox::intersect(int x, int y, int r, int t){
	if(x>x_)
		x_=x;
	if(y>y_)
		y_=y;
	if(r<r_)
		r_=r;
	if(t<t_)
		t_=t;
}

bool IntegerBox::isContained(int x,int y,int r,int t){
	bool stillgood=true;
	if(x<this->x())
		stillgood=false;
	if(y<this->y())
		stillgood=false;
	if(r>this->range())
		stillgood=false;
	if(t>this->top())
		stillgood=false;
	return stillgood;

}
bool IntegerBox::isContained(const IntegerBox b){

	int x=b.x();
	int y=b.y();
	int t=b.top();
	int r=b.range();
	bool stillgood=true;
	if(x<this->x())
		stillgood=false;
	if(y<this->y())
		stillgood=false;
	if(r>this->range())
		stillgood=false;
	if(t>this->top())
		stillgood=false;
	return stillgood;
}
