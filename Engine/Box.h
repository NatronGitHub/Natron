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

#ifndef POWITER_ENGINE_BOX_H_
#define POWITER_ENGINE_BOX_H_

#include <cassert>

// the y coordinate is towards the top
// the x coordinate is towards the right
class Box2D{
private:
	int _l; // left
	int _b; // bottom
	int _r; // right
	int _t; // top
    

public:
    
    
    /*iterator : bottom-top, left-right */
    class  iterator {
    public:
		// current position
        int y;
        int x;
        
        iterator(int y, int x, int l, int r) : y(y), x(x), l(l), r(r) { }
        
        void operator++() {
            ++x;
            if (x == r) {
                x = l;
                ++y;
            }
        }
        
        void operator++(int) {
            ++x;
            if (x == r) {
                x = l;
                ++y;
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
        return iterator(_b, _l, _l, _r);
    }
    
    iterator end() const
    {
        return iterator(_t, _l, _l, _r);
    }

    // default cionstructor: empty box
    Box2D() : _l(0), _b(0), _r(0), _t(0) {}
    
    Box2D(int l, int b, int r, int t) : _l(l), _b(b), _r(r), _t(t) { assert((_r>= _l) && (_t>=_b)); }
    
    explicit Box2D(const Box2D &b):_l(b._l),_b(b._b),_r(b._r),_t(b._t) { assert((_r>= _l) && (_t>=_b)); }

    int left() const { return _l; }
    void set_left(int v) { _l = v; }
    
    int bottom() const { return _b; } 
    void set_bottom(int v) { _b = v; }
    
    int right() const { return _r; } 
    void set_right(int v) { _r = v; }
    
    int top() const { return _t; } 
    void set_top(int v) { _t = v; }
    
    int width() const { return _r - _l; }
    
    int height() const { return _t - _b; } 

    double middle_row() const { return (_l + _r) / 2.0f; }
    
    double middle_column() const { return (_b + _t) / 2.0f; }
    
   
    void set(int l, int b, int r, int t) {
        _l = l;
        _b = b;
        _r = r;
        _t = t;
        assert((_r>= _l) && (_t>=_b)); 
    }
    
   
    void set(const Box2D& b) { *this = b; }
    
    
    //bool is1x1() const { return (_r == _l + 1) && _t == _b + 1; }
    
    bool isEmpty() const { return (_r <= _l) || (_t <= _b); }
    
    // reset to an empty box
    void clear() {
        _l = _b = 0;
        _r = _t = 0;
    }
    
    void move(int dx, int dy) {
        _l += dx;
        _r += dx;
        _b += dy;
        _t += dy;
    }
    
    /*Pad the edges of the box by the input values.
     *This is used by filters that increases the BBOX 
     *of the image.*/
    void pad(int dl, int db, int dr, int dt)
    {
        if (_r + dr >= _l + dl) {
            _l += dl;
            _r += dr;
        }
        if (_t + dt >= _b + db) {
            _b += db;
            _t += dt;
        }
    }
    
    /*clamp x to the region [_l,_r-1] */
    int clampx(int x) const { return x <= _l ? _l : x >= _r ? _r - 1 : x; }
    
   
    int clampy(int y) const { return y <= _b ? _b : y >= _t ? _t - 1 : y; }
    
	/*merge the current box with another integerBox.
	 *The current box is the smallest box enclosing the two boxes
     (not the union, which is not a box).*/
    void merge(const Box2D& box) {
        merge(box.left(), box.bottom(), box.right(), box.top());
    }

    void merge(int l, int b, int r, int t) {
        if (l < left()) {
            _l = l;
        }
        if (b < bottom()) {
            _b = b;
        }
        if (r > right()) {
            _r = r;
        }
        if (t > top()) {
            _t = t;
        }
    }

    
	/*intersection of two boxes*/
    void intersect(const Box2D& box) {
        intersect(box.left(), box.bottom(), box.right(), box.top());
    }

    
    void intersect(int l, int b, int r, int t) {
        if (l > left()) {
            _l = l;
        }
        if (b > bottom()) {
            _b = b;
        }
        if (r < right()) {
            _r = r;
        }
        if (t < top()) {
            _t = t;
        }
    }


    /// returns true if the Box2D passed as parameter is fully contained in this one
    bool isContained(const Box2D& b) const {
        return b.isEmpty() || ((b.left() >= left()) && (b.bottom() > bottom()) &&
                               (b.right() <= right()) && (b.top() <= top()));
    }

    bool isContained(int l,int b,int r,int t) const {
        return isContained(Box2D(l,b,r,t));
    }
    
	/*the area : w*h*/
    int area() const {
        return width() * height();
    }
    Box2D& operator=(const Box2D& other){
        _l = other.left();
        _b = other.bottom();
        _r = other.right();
        _t = other.top();
        return *this;
    }
};
/// equality of boxes
inline bool operator==(const Box2D& b1, const Box2D& b2)
{
	return b1.left() == b2.left() &&
    b1.bottom() == b2.bottom() &&
    b1.right() == b2.right() &&
    b1.top() == b2.top();
}

/// inequality of boxes
inline bool operator!=(const Box2D& b1, const Box2D& b2)
{
	return b1.left() != b2.left() ||
    b1.bottom() != b2.bottom() ||
    b1.right() != b2.right() ||
    b1.top() != b2.top();
}

#endif