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

 

 



#ifndef __INTEGER_BOX_H__
#define __INTEGER_BOX_H__

class Box2D{
private:
	int _x; // left
	int _y; // bottom
	int _r; // right
	int _t; // top
    
protected:
    //protected so Node::Info can modify it
    bool _modified; // true when some functions are called
public:
    
    bool hasBeenModified(){return _modified;}
    
    /*iterator : bottom-top, left-right */
    class  iterator {
    public:
		// current position
        int y;
        int x;
        
        iterator(int y, int x, int l, int r) : y(y), x(x), l(l), r(r) { }
        
        void operator++() {
            x++;
            if (x == r) {
                x = l;
                y++;
            }
        }
        
        void operator++(int) {
            x++;
            if (x == r) {
                x = l;
                y++;
            }
        }
        
        bool operator!=(const iterator& other) const
        {
            return y != other.y || x != other.x;
        }
        
        
    private:
        int l;
        int r;
    };
    
    typedef iterator const_iterator;
    
    iterator begin() const
    {
        return iterator(_y, _x, _x, _r);
    }
    
    iterator end() const
    {
        return iterator(_t, _x, _x, _r);
    }
    
    Box2D() : _x(0), _y(0), _r(1), _t(1) ,_modified(false){}
    
    Box2D(int x, int y, int r, int t) : _x(x), _y(y), _r(r), _t(t) , _modified(false){}
    
    Box2D(const Box2D &b):_x(b._x),_y(b._y),_r(b._r),_t(b._t) , _modified(false){}
    
    int x() const { return _x; }
    void x(int v) { _x = v; }
    
    int y() const { return _y; } 
    void y(int v) { _y = v; }
    
    int right() const { return _r; } 
    void right(int v) { _r = v; } 
    
    int top() const { return _t; } 
    void top(int v) { _t = v; } 
    
    int w() const { return _r - _x; }
    void w(int v) { _r = _x + v; } 
    
    int h() const { return _t - _y; } 
    void h(int v) { _t = _y + v; }

    float middle_row() const { return (_x + _r) / 2.0f; }
    
    float middle_column() const { return (_y + _t) / 2.0f; }
    
   
    void set(int x, int y, int r, int t) { _x = x;
        _y = y;
        _r = r;
        _t = t;
        _modified = true;
    }
    
   
    void set(const Box2D& b) { *this = b; }
    
    
    bool is1x1() const { return _r <= _x + 1 && _t <= _y + 1; }
    
    void clear() { _x = _y = 0;
        _r = _t = 1; }      
    
    void move(int dx, int dy) { _x += dx;
        _r += dx;
        _y += dy;
        _t += dy;
        _modified = true;
    }
    
    /*Pad the edges of the box by the input values.
     *This is used by filters that increases the BBOX 
     *of the image.*/
    void pad(int dx, int dy, int dr, int dt)
    {
        if (_r > _x + 1) { _x += dx;
            _r += dr; }
        if (_t > _y + 1) { _y += dy;
            _t += dt; }
        _modified = true;
    }
    
    /*add the same amount left/right and bottom/top. */
    void pad(int dx, int dy) { pad(-dx, -dy, dx, dy); }
    
    /* add the same amount everywhere */
    void pad(int d) { pad(-d, -d, d, d); }
    
    /*clamp x to the region [_x,_r-1] */
    int clampx(int x) const { return x <= _x ? _x : x >= _r ? _r - 1 : x; }
    
   
    int clampy(int y) const { return y <= _y ? _y : y >= _t ? _t - 1 : y; }
    
	/*merge the current box with another integerBox.
	 *The current box is the union of the two boxes.*/
    void merge(const Box2D&);
    
    void merge(int x, int y);
    
    void merge(int x, int y, int r, int t);
    
	/*intersection of two boxes*/
    void intersect(const Box2D);
    
    void intersect(int x, int y, int r, int t);
    
    bool isContained(const Box2D);
    
    bool isContained(int x,int y,int r,int t);
    
	/*the area : w*h*/
    int area() const {
        return w() * h();
    }
    Box2D& operator=(const Box2D& other){
        _x = other.x();
        _y = other.y();
        _r = other.right();
        _t = other.top();
        return *this;
    }
};
/// equality of boxes
inline bool operator==(const Box2D& b1, const Box2D& b2)
{
	return b1.x() == b2.x() &&
    b1.y() == b2.y() &&
    b1.right() == b2.right() &&
    b1.top() == b2.top();
}

/// inequality of boxes
inline bool operator!=(const Box2D& b1, const Box2D& b2)
{
	return b1.x() != b2.x() ||
    b1.y() != b2.y() ||
    b1.right() != b2.right() ||
    b1.top() != b2.top();
}

#endif