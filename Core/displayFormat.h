#ifndef __DISPLAY_FORMAT_H__
#define __DISPLAY_FORMAT_H__

#include <map>
#include "Core/Box.h"

class DisplayFormat:public IntegerBox{
	
	
public:
	DisplayFormat(int x,int y,int r,int t,const char* name,double pa=1.0);
    DisplayFormat(const DisplayFormat& other):IntegerBox(other.x(),other.y(),other.range(),other.top()),_name(other.name()),_pixel_aspect(other.pixel_aspect()){}
  
    DisplayFormat();
    virtual ~DisplayFormat(){
        //delete [] _name;
    }
    const char* name() const ;
    void name(const char* n){this->_name = n;}
    const double pixel_aspect() const;
    void pixel_aspect( float p) ;
	
	DisplayFormat& operator=(const DisplayFormat& other){
        set(other.x(), other.y(), other.range(), other.top());
        name(other.name());
        pixel_aspect(other.pixel_aspect());
        return *this;
    }
    bool operator==(const DisplayFormat& other){
        return _pixel_aspect == other.pixel_aspect() &&
        x() == other.x() &&
        y() == other.y() &&
        range() == other.range() &&
        top() == other.top();
    }
    
private:
	double _pixel_aspect;
    const char* _name;
};


#endif // __DISPLAY_FORMAT_H__