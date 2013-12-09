//  Natron
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 *Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 *contact: immarespond at gmail dot com
 *
 */

#ifndef NATRON_ENGINE_RECT_H_
#define NATRON_ENGINE_RECT_H_

#include <cassert>
#include <iostream>
#include <vector>
#include <utility>

#include <boost/archive/xml_iarchive.hpp>
#include <boost/archive/xml_oarchive.hpp>

#include "Global/Macros.h"

GCC_DIAG_OFF(strict-overflow)
/**
 * @brief A rectangle where _l < _r and _b < _t such as width() == (_r - _l) && height() == (_t - _b)
 **/
class RectI{
private:
	int _l; // left
	int _b; // bottom
	int _r; // right
	int _t; // top
    
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version)
    {
        (void)version;
        ar & boost::serialization::make_nvp("Left",_l);
        ar & boost::serialization::make_nvp("Bottom",_b);
        ar & boost::serialization::make_nvp("Right",_r);
        ar & boost::serialization::make_nvp("Top",_t);
        
    }
public:
    
    RectI() : _l(0), _b(0), _r(0), _t(0) {}
    
    RectI(int l, int b, int r, int t) : _l(l), _b(b), _r(r), _t(t) { assert((_r>= _l) && (_t>=_b)); }
    
    RectI(const RectI &b):_l(b._l),_b(b._b),_r(b._r),_t(b._t) { assert((_r>= _l) && (_t>=_b)); }
    
    virtual ~RectI(){}
    
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
    
    void set(int l, int b, int r, int t) {
        _l = l;
        _b = b;
        _r = r;
        _t = t;
        assert((_r>= _l) && (_t>=_b));
    }
    
    
    void set(const RectI& b) { *this = b; }
    
    
    bool isNull() const { return (_r <= _l) || (_t <= _b); }
    
    operator bool() const { return !isNull(); }
    
    RectI operator&(const RectI& other) const{
        RectI inter;
        intersect(other,&inter);
        return inter;
    }
    
    void clear() {
        _l = 0;
        _b = 0;
        _r = 0;
        _t = 0;
    }
    
	/*merge the current box with another integerBox.
	 *The current box is the smallest box enclosing the two boxes
     (not the union, which is not a box).*/
    void merge(const RectI& box) {
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
    bool intersect(const RectI& box,RectI* intersection) const {
        return intersect(box.left(), box.bottom(), box.right(), box.top(),intersection);
    }
    
    
    bool intersect(int l, int b, int r, int t,RectI* intersection) const {
        if(r < _l || l > _r || b > _t || t < _b)
            return false;
        intersection->set_left(std::max(_l, l));
        intersection->set_right(std::min(_r, r));
        intersection->set_bottom(std::max(_b, b));
        intersection->set_top(std::min(_t, t));
        return true;
    }
    
    
    /// returns true if the rect passed as parameter is fully contained in this one
    bool intersects(const RectI& b) const {
        return b.isNull() || ((b.left() >= left()) && (b.bottom() > bottom()) &&
                              (b.right() <= right()) && (b.top() <= top()));
    }
    
    bool intersects(int l,int b,int r,int t) const {
        return intersects(RectI(l,b,r,t));
    }
    
	/*the area : w*h*/
    int area() const {
        return width() * height();
    }
    RectI& operator=(const RectI& other){
        _l = other.left();
        _b = other.bottom();
        _r = other.right();
        _t = other.top();
        return *this;
    }
    
    bool contains(const RectI& other) const {
        return other._l >= _l &&
        other._b >= _b &&
        other._r <= _r &&
        other._t <= _t;
    }
    
    void debug() const{
        std::cout << "RectI is..." << std::endl;
        std::cout << "left = " << _l << std::endl;
        std::cout << "bottom = " << _b << std::endl;
        std::cout << "right = " << _r << std::endl;
        std::cout << "top = " << _t << std::endl;
    }

    static std::vector<RectI> splitRectIntoSmallerRect(const RectI& rect,int splitsCount) {
        std::vector<RectI> ret;
        if (rect.isNull()) {
            return ret;
        }
        int averagePixelsPerSplit = std::ceil(double(rect.area()) / (double)splitsCount);
        /*if the splits happen to have less pixels than 1 scan-line contains, just do scan-line rendering*/
        if(averagePixelsPerSplit < rect.width()){
            for (int i = rect.bottom(); i < rect.top(); ++i) {
                ret.push_back(RectI(rect.left(),i,rect.right(),i+1));
            }
        }else{
            //we round to the ceil
            int scanLinesCount = std::ceil((double)averagePixelsPerSplit/(double)rect.width());
            int startBox = rect.bottom();
            while (startBox < rect.top() - scanLinesCount) {
                ret.push_back(RectI(rect.left(),startBox,rect.right(),startBox+scanLinesCount));
                startBox += scanLinesCount;
            }
            if(startBox < rect.top()){
                ret.push_back(RectI(rect.left(),startBox,rect.right(),rect.top()));
            }
        }
        return ret;
    }
};
GCC_DIAG_ON(strict-overflow)
    

/// equality of boxes
inline bool operator==(const RectI& b1, const RectI& b2)
{
	return b1.left() == b2.left() &&
    b1.bottom() == b2.bottom() &&
    b1.right() == b2.right() &&
    b1.top() == b2.top();
}

/// inequality of boxes
inline bool operator!=(const RectI& b1, const RectI& b2)
{
	return b1.left() != b2.left() ||
    b1.bottom() != b2.bottom() ||
    b1.right() != b2.right() ||
    b1.top() != b2.top();
}
BOOST_SERIALIZATION_ASSUME_ABSTRACT(RectI);


class RectD {
    double _l; // left
    double _b; // bottom
    double _r; // right
    double _t; // top
    
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version)
    {
        (void)version;
        ar & boost::serialization::make_nvp("Left",_l);
        ar & boost::serialization::make_nvp("Bottom",_b);
        ar & boost::serialization::make_nvp("Right",_r);
        ar & boost::serialization::make_nvp("Top",_t);
        
    }
public:
    
    RectD() : _l(0), _b(0), _r(0), _t(0) {}
    
    RectD(double l, double b, double r, double t) : _l(l), _b(b), _r(r), _t(t) { assert((_r>= _l) && (_t>=_b)); }
    
    RectD(const RectD &b):_l(b._l),_b(b._b),_r(b._r),_t(b._t) { assert((_r>= _l) && (_t>=_b)); }
    
    virtual ~RectD(){}
    
    double left() const { return _l; }
    void set_left(double v) { _l = v; }
    
    double bottom() const { return _b; }
    void set_bottom(double v) { _b = v; }
    
    double right() const { return _r; }
    void set_right(double v) { _r = v; }
    
    double top() const { return _t; }
    void set_top(double v) { _t = v; }
    
    double width() const { return _r - _l; }
    
    double height() const { return _t - _b; }
    
    
    
    void set(double l, double b, double r, double t) {
        _l = l;
        _b = b;
        _r = r;
        _t = t;
        assert((_r>= _l) && (_t>=_b));
    }
    
    
    void set(const RectD& b) { *this = b; }
    
    
    bool isNull() const { return (_r <= _l) || (_t <= _b); }
    
    operator bool() const { return !isNull(); }
    
    RectD operator&(const RectD& other) const{
        RectD inter;
        intersect(other,&inter);
        return inter;
    }
    
    void clear() {
        _l = 0;
        _b = 0;
        _r = 0;
        _t = 0;
    }
    
    /*merge the current box with another integerBox.
     *The current box is the smallest box enclosing the two boxes
     (not the union, which is not a box).*/
    void merge(const RectD& box) {
        merge(box.left(), box.bottom(), box.right(), box.top());
    }
    
    void merge(double l, double b, double r, double t) {
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
    bool intersect(const RectD& box,RectD* intersection) const {
        return intersect(box.left(), box.bottom(), box.right(), box.top(),intersection);
    }
    
    
    bool intersect(double l, double b, double r, double t,RectD* intersection) const {
        if(r < _l || l > _r || b > _t || t < _b)
            return false;
        intersection->set_left(std::max(_l, l));
        intersection->set_right(std::min(_r, r));
        intersection->set_bottom(std::max(_b, b));
        intersection->set_top(std::min(_t, t));
        return true;
    }
    
    
    /// returns true if the rect passed as parameter is fully contained in this one
    bool intersects(const RectD& b) const {
        return b.isNull() || ((b.left() >= left()) && (b.bottom() > bottom()) &&
                              (b.right() <= right()) && (b.top() <= top()));
    }
    
    bool intersects(double l,double b,double r,double t) const {
        return intersects(RectD(l,b,r,t));
    }
    
    /*the area : w*h*/
    double area() const {
        return width() * height();
    }
    RectD& operator=(const RectD& other){
        _l = other.left();
        _b = other.bottom();
        _r = other.right();
        _t = other.top();
        return *this;
    }
    
    bool contains(const RectD& other) const {
        return other._l >= _l &&
        other._b >= _b &&
        other._r <= _r &&
        other._t <= _t;
    }
    
    void debug() const{
        std::cout << "RectI is..." << std::endl;
        std::cout << "left = " << _l << std::endl;
        std::cout << "bottom = " << _b << std::endl;
        std::cout << "right = " << _r << std::endl;
        std::cout << "top = " << _t << std::endl;
    }
    
    
    
};

/// equality of boxes
inline bool operator==(const RectD& b1, const RectD& b2)
{
	return b1.left() == b2.left() &&
    b1.bottom() == b2.bottom() &&
    b1.right() == b2.right() &&
    b1.top() == b2.top();
}

/// inequality of boxes
inline bool operator!=(const RectD& b1, const RectD& b2)
{
	return b1.left() != b2.left() ||
    b1.bottom() != b2.bottom() ||
    b1.right() != b2.right() ||
    b1.top() != b2.top();
}
BOOST_SERIALIZATION_ASSUME_ABSTRACT(RectD);


#endif // NATRON_ENGINE_RECT_H_
