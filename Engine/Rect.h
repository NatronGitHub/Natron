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
 * @brief A rectangle where x1 < x2 and y1 < y2 such as width() == (x2 - x1) && height() == (y2 - y1)
 **/
class RectI{

public:
    
    ////public so the fields can be access exactly like the OfxRect struct !
    int x1; // left
    int y1; // bottom
    int x2; // right
    int y2; // top
    
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version)
    {
        (void)version;
        ar & boost::serialization::make_nvp("Left",x1);
        ar & boost::serialization::make_nvp("Bottom",y1);
        ar & boost::serialization::make_nvp("Right",x2);
        ar & boost::serialization::make_nvp("Top",y2);
        
    }
    
    RectI() : x1(0), y1(0), x2(0), y2(0) {}
    
    RectI(int l, int b, int r, int t) : x1(l), y1(b), x2(r), y2(t) { assert((x2>= x1) && (y2>=y1)); }
    
    RectI(const RectI &b):x1(b.x1),y1(b.y1),x2(b.x2),y2(b.y2) { assert((x2>= x1) && (y2>=y1)); }
    
    virtual ~RectI(){}
    
    int left() const { return x1; }
    void set_left(int v) { x1 = v; }
    
    int bottom() const { return y1; }
    void set_bottom(int v) { y1 = v; }
    
    int right() const { return x2; }
    void set_right(int v) { x2 = v; }
    
    int top() const { return y2; }
    void set_top(int v) { y2 = v; }
    
    int width() const { return x2 - x1; }
    
    int height() const { return y2 - y1; }
    
    void set(int l, int b, int r, int t) {
        x1 = l;
        y1 = b;
        x2 = r;
        y2 = t;
        assert((x2>= x1) && (y2>=y1));
    }
    
    
    void set(const RectI& b) { *this = b; }
    
    
    bool isNull() const { return (x2 <= x1) || (y2 <= y1); }
    
    operator bool() const { return !isNull(); }
    
    void clear() {
        x1 = 0;
        y1 = 0;
        x2 = 0;
        y2 = 0;
    }
    
	/*merge the current box with another integerBox.
	 *The current box is the smallest box enclosing the two boxes
     (not the union, which is not a box).*/
    void merge(const RectI& box) {
        merge(box.left(), box.bottom(), box.right(), box.top());
    }
    
    void merge(int l, int b, int r, int t) {
        if (l < left()) {
            x1 = l;
        }
        if (b < bottom()) {
            y1 = b;
        }
        if (r > right()) {
            x2 = r;
        }
        if (t > top()) {
            y2 = t;
        }
    }
    
	/*intersection of two boxes*/
    bool intersect(const RectI& r,RectI* intersection) const {
        if (isNull() || r.isNull())
            return false;
   
        if (x1 > r.x2 || r.x1 > x2 || y1 > r.y2 || r.y1 > y2)
            return false;
    
        intersection->x1 = std::max(x1,r.x1);
        intersection->x2 = std::min(x2,r.x2);
        intersection->y1 = std::max(y1,r.y1);
        intersection->y2 = std::min(y2,r.y2);
        return true;
    }
    
    
    bool intersect(int l, int b, int r, int t,RectI* intersection) const {
        return intersect(RectI(l,b,r,t),intersection);
        
    }
    
    
    /// returns true if the rect passed as parameter is intersects this one
    bool intersects(const RectI& r) const {
        if(isNull() || r.isNull()){
            return false;
        }
        if (x1 > r.x2 || r.x1 > x2 || y1 > r.y2 || r.y1 > y2)
            return false;
        return true;
    }
    
    bool intersects(int l,int b,int r,int t) const {
        return intersects(RectI(l,b,r,t));
    }
    
	/*the area : w*h*/
    int area() const {
        return width() * height();
    }
    RectI& operator=(const RectI& other){
        x1 = other.left();
        y1 = other.bottom();
        x2 = other.right();
        y2 = other.top();
        return *this;
    }
    
    bool contains(const RectI& other) const {
        return other.x1 >= x1 &&
        other.y1 >= y1 &&
        other.x2 <= x2 &&
        other.y2 <= y2;
    }
    
    bool contains(int x,int y) const {
        return x >= x1 && x < x2 && y >= y1 && y < y2;
    }
    
    void move(int dx,int dy) {
        x1 += dx;
        y1 += dy;
        x2 += dx;
        y2 += dy;
    }
    
    void debug() const{
        std::cout << "RectI is..." << std::endl;
        std::cout << "left = " << x1 << std::endl;
        std::cout << "bottom = " << y1 << std::endl;
        std::cout << "right = " << x2 << std::endl;
        std::cout << "top = " << y2 << std::endl;
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
    double x1; // left
    double y1; // bottom
    double x2; // right
    double y2; // top
    
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version)
    {
        (void)version;
        ar & boost::serialization::make_nvp("Left",x1);
        ar & boost::serialization::make_nvp("Bottom",y1);
        ar & boost::serialization::make_nvp("Right",x2);
        ar & boost::serialization::make_nvp("Top",y2);
        
    }
public:
    
    RectD() : x1(0), y1(0), x2(0), y2(0) {}
    
    RectD(double l, double b, double r, double t) : x1(l), y1(b), x2(r), y2(t) { assert((x2>= x1) && (y2>=y1)); }
    
    RectD(const RectD &b):x1(b.x1),y1(b.y1),x2(b.x2),y2(b.y2) { assert((x2>= x1) && (y2>=y1)); }
    
    virtual ~RectD(){}
    
    double left() const { return x1; }
    void set_left(double v) { x1 = v; }
    
    double bottom() const { return y1; }
    void set_bottom(double v) { y1 = v; }
    
    double right() const { return x2; }
    void set_right(double v) { x2 = v; }
    
    double top() const { return y2; }
    void set_top(double v) { y2 = v; }
    
    double width() const { return x2 - x1; }
    
    double height() const { return y2 - y1; }
    
    
    
    void set(double l, double b, double r, double t) {
        x1 = l;
        y1 = b;
        x2 = r;
        y2 = t;
        assert((x2>= x1) && (y2>=y1));
    }
    
    
    void set(const RectD& b) { *this = b; }
    
    
    bool isNull() const { return (x2 <= x1) || (y2 <= y1); }
    
    operator bool() const { return !isNull(); }
    
    void clear() {
        x1 = 0;
        y1 = 0;
        x2 = 0;
        y2 = 0;
    }
    
    /*merge the current box with another integerBox.
     *The current box is the smallest box enclosing the two boxes
     (not the union, which is not a box).*/
    void merge(const RectD& box) {
        merge(box.left(), box.bottom(), box.right(), box.top());
    }
    
    void merge(double l, double b, double r, double t) {
        if (l < left()) {
            x1 = l;
        }
        if (b < bottom()) {
            y1 = b;
        }
        if (r > right()) {
            x2 = r;
        }
        if (t > top()) {
            y2 = t;
        }
    }
    
    /*intersection of two boxes*/
    bool intersect(const RectD& r,RectD* intersection) const {
        if (isNull() || r.isNull())
            return false;
        
        if (x1 > r.x2 || r.x1 > x2 || y1 > r.y2 || r.y1 > y2)
            return false;
        
        intersection->x1 = std::max(x1,r.x1);
        intersection->x2 = std::min(x2,r.x2);
        intersection->y1 = std::max(y1,r.y1);
        intersection->y2 = std::min(y2,r.y2);
        return true;
    }
    
    
    bool intersect(int l, int b, int r, int t,RectD* intersection) const {
        return intersect(RectD(l,b,r,t),intersection);
        
    }
    
    
    /// returns true if the rect passed as parameter is intersects this one
    bool intersects(const RectD& r) const {
        if(isNull() || r.isNull()){
            return false;
        }
        if (x1 > r.x2 || r.x1 > x2 || y1 > r.y2 || r.y1 > y2)
            return false;
        return true;
    }
    
    bool intersects(int l,int b,int r,int t) const {
        return intersects(RectD(l,b,r,t));
    }
    
    
    /*the area : w*h*/
    double area() const {
        return width() * height();
    }
    RectD& operator=(const RectD& other){
        x1 = other.left();
        y1 = other.bottom();
        x2 = other.right();
        y2 = other.top();
        return *this;
    }
    
    bool contains(const RectD& other) const {
        return other.x1 >= x1 &&
        other.y1 >= y1 &&
        other.x2 <= x2 &&
        other.y2 <= y2;
    }
    
    void debug() const{
        std::cout << "RectI is..." << std::endl;
        std::cout << "left = " << x1 << std::endl;
        std::cout << "bottom = " << y1 << std::endl;
        std::cout << "right = " << x2 << std::endl;
        std::cout << "top = " << y2 << std::endl;
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
