//  Natron
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 * Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 * contact: immarespond at gmail dot com
 *
 */

#ifndef NATRON_ENGINE_RECT_H_
#define NATRON_ENGINE_RECT_H_

// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>

#include <cassert>
#include <iostream>
#include <vector>
#include <utility>
#include <cmath>
#include <algorithm> // min, max
#include <limits>

#include "Global/GlobalDefines.h"

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#pragma message WARN("move serialization to a separate header")
CLANG_DIAG_OFF(unused-local-typedef)
GCC_DIAG_OFF(unused-parameter)
// /opt/local/include/boost/serialization/smart_cast.hpp:254:25: warning: unused parameter 'u' [-Wunused-parameter]
#include <boost/archive/xml_iarchive.hpp>
#include <boost/archive/xml_oarchive.hpp>
CLANG_DIAG_ON(unused-local-typedef)
GCC_DIAG_ON(unused-parameter)
#endif

#include "Global/Macros.h"

GCC_DIAG_OFF(strict-overflow)

class RectD;

/**
 * @brief A rectangle where x1 < x2 and y1 < y2 such as width() == (x2 - x1) && height() == (y2 - y1)
 **/
class RectI
{
public:

    ////public so the fields can be access exactly like the OfxRect struct !
    int x1; // left
    int y1; // bottom
    int x2; // right
    int y2; // top

    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar,
                   const unsigned int version)
    {
        Q_UNUSED(version);
        ar & boost::serialization::make_nvp("Left",x1);
        ar & boost::serialization::make_nvp("Bottom",y1);
        ar & boost::serialization::make_nvp("Right",x2);
        ar & boost::serialization::make_nvp("Top",y2);
    }

    RectI()
        : x1(0), y1(0), x2(0), y2(0)
    {
    }

    RectI(int l,
          int b,
          int r,
          int t)
        : x1(l), y1(b), x2(r), y2(t)
    {
        assert( (x2 >= x1) && (y2 >= y1) );
    }

    RectI(const RectI &b)
        : x1(b.x1),y1(b.y1),x2(b.x2),y2(b.y2)
    {
        assert( (x2 >= x1) && (y2 >= y1) );
    }

    virtual ~RectI()
    {
    }

    int left() const
    {
        return x1;
    }

    void set_left(int v)
    {
        x1 = v;
    }

    int bottom() const
    {
        return y1;
    }

    void set_bottom(int v)
    {
        y1 = v;
    }

    int right() const
    {
        return x2;
    }

    void set_right(int v)
    {
        x2 = v;
    }

    int top() const
    {
        return y2;
    }

    void set_top(int v)
    {
        y2 = v;
    }

    int width() const
    {
        return x2 - x1;
    }

    int height() const
    {
        return y2 - y1;
    }

    void set(int l,
             int b,
             int r,
             int t)
    {
        x1 = l;
        y1 = b;
        x2 = r;
        y2 = t;
        /*assert( (x2 >= x1) && (y2 >= y1) );*/
    }

    void set(const RectI & b)
    {
        *this = b;
    }
    

    /**
     * @brief Upscales the bounds assuming this rectangle is the Nth level of mipmap
     **/
    RectI upscalePowerOfTwo(unsigned int thisLevel) const
    {
        if (thisLevel == 0) {
            return *this;
        }
        RectI ret;
        ret.x1 = x1 << thisLevel;
        ret.x2 = x2 << thisLevel;
        ret.y1 = y1 << thisLevel;
        ret.y2 = y2 << thisLevel;

        return ret;
    }

    inline void toCanonical(unsigned int thisLevel, double par, const RectD & rod, RectD *rect) const;
    inline void toCanonical_noClipping(unsigned int thisLevel, double par, RectD *rect) const;

    // the following should never be used: only canonical coordinates may be downscaled
    /**
     * @brief Scales down the rectangle by the given power of 2
     **/
    RectI downscalePowerOfTwo(unsigned int thisLevel) const
    {
        if (thisLevel == 0) {
            return *this;
        }
        RectI ret;
        assert(x1 % (1 << thisLevel) == 0 && x2 % (1 << thisLevel) == 0 && y1 % (1 << thisLevel) == 0 && y2 % (1 << thisLevel) == 0);
        ret.x1 = x1 >> thisLevel;
        ret.x2 = x2 >> thisLevel;
        ret.y1 = y1 >> thisLevel;
        ret.y2 = y2 >> thisLevel;

        return ret;
    }

    /*
       test program for rounding integer to the next/previous pot:
       #include <stdio.h>
       int main()
       {
       int i;
       int pot = 3;
       int scale = 1 << pot;
       int scalem1 = scale - 1;
       for(i=-100; i < 100; ++i)
       {
         printf("%d => %d,%d %d,%d\n", i, i & ~scalem1, i+scalem1 & ~scalem1, (i >> pot) << pot, ((i+scalem1)>>pot) << pot);
       }
       }
     */
    /**
     * @brief round the rectangle by the given power of 2, and return the largest *enclosed* (inside) rectangle
     **/
    RectI roundPowerOfTwoLargestEnclosed(unsigned int thisLevel) const
    {
        if (thisLevel == 0) {
            return *this;
        }
        RectI ret;
        int pot = (1 << thisLevel);
        int pot_minus1 = pot - 1;
        ret.x1 = (x1 + pot_minus1) & ~pot_minus1;
        ret.x2 = x2 & ~pot_minus1;
        ret.y1 = (y1 + pot_minus1) & ~pot_minus1;
        ret.y2 = y2 & ~pot_minus1;
        // check that it's enclosed
        assert(ret.x1 >= x1 && ret.x2 <= x2 && ret.y1 >= y1 && ret.y2 <= y2);

        return ret;
    }

    /**
     * @brief round the rectangle by the given power of 2, and return the smallest *enclosing* rectangle
     **/
    RectI roundPowerOfTwoSmallestEnclosing(unsigned int thisLevel) const
    {
        if (thisLevel == 0) {
            return *this;
        }
        RectI ret;
        int pot = (1 << thisLevel);
        int pot_minus1 = pot - 1;
        ret.x1 = x1 & ~pot_minus1;
        ret.x2 = (x2 + pot_minus1) & ~pot_minus1;
        ret.y1 = y1 & ~pot_minus1;
        ret.y2 = (y2 + pot_minus1) & ~pot_minus1;
        // check that it's enclosing
        assert(ret.x1 <= x1 && ret.x2 >= x2 && ret.y1 <= y1 && ret.y2 >= y2);

        return ret;
    }

    /**
     * @brief Scales down the rectangle by the given power of 2, and return the largest *enclosed* (inside) rectangle
     **/
    RectI downscalePowerOfTwoLargestEnclosed(unsigned int thisLevel) const
    {
        if (thisLevel == 0) {
            return *this;
        }
        RectI ret;
        int pot = (1 << thisLevel);
        int pot_minus1 = pot - 1;
        ret.x1 = (x1 + pot_minus1) >> thisLevel;
        ret.x2 = x2 >> thisLevel;
        ret.y1 = (y1 + pot_minus1) >> thisLevel;
        ret.y2 = y2 >> thisLevel;
        // check that it's enclosed
        assert(ret.x1 * pot >= x1 && ret.x2 * pot <= x2 && ret.y1 * pot >= y1 && ret.y2 * pot <= y2);

        return ret;
    }

    /**
     * @brief Scales down the rectangle in pixel coordinates by the given power of 2, and return the smallest *enclosing* rectangle in pixel coordinates
     **/
    RectI downscalePowerOfTwoSmallestEnclosing(unsigned int thisLevel) const
    {
        if (thisLevel == 0) {
            return *this;
        }
        RectI ret;
        int pot = (1 << thisLevel);
        int pot_minus1 = pot - 1;
        ret.x1 = x1 >> thisLevel;
        ret.x2 = (x2 + pot_minus1) >> thisLevel;
        ret.y1 = y1 >> thisLevel;
        ret.y2 = (y2 + pot_minus1) >> thisLevel;
        // check that it's enclosing
        assert(ret.x1 * pot <= x1 && ret.x2 * pot >= x2 && ret.y1 * pot <= y1 && ret.y2 * pot >= y2);

        return ret;
    }

    bool isNull() const
    {
        return (x2 <= x1) || (y2 <= y1);
    }

    bool isInfinite() const
    {
        return x1 <= kOfxFlagInfiniteMin || x2 >= kOfxFlagInfiniteMax || y1 <= kOfxFlagInfiniteMin || y2 >= kOfxFlagInfiniteMax;
    }

    void clear()
    {
        x1 = 0;
        y1 = 0;
        x2 = 0;
        y2 = 0;
    }

    /*merge the current box with another integerBox.
     * The current box is the smallest box enclosing the two boxes
       (not the union, which is not a box).*/
    void merge(const RectI & box)
    {
        merge( box.left(), box.bottom(), box.right(), box.top() );
    }

    void merge(int l,
               int b,
               int r,
               int t)
    {
        x1 = std::min(x1, l);
        x2 = std::max(x2, r);
        y1 = std::min(y1, b);
        y2 = std::max(y2, t);

    }

    /*intersection of two boxes*/
    bool intersect(const RectI & r,
                   RectI* intersection) const
    {
        if ( !intersects(r) ) {
            return false;
        }

        intersection->x1 = std::max(x1,r.x1);
        // the region must be *at least* empty, thus the maximin.
        intersection->x2 = std::max(intersection->x1,std::min(x2,r.x2));
        intersection->y1 = std::max(y1,r.y1);
        // the region must be *at least* empty, thus the maximin.
        intersection->y2 = std::max(intersection->y1,std::min(y2,r.y2));

        assert(!intersection->isNull());
        
        return true;
    }

    bool intersect(int l,
                   int b,
                   int r,
                   int t,
                   RectI* intersection) const
    {
        return intersect(RectI(l,b,r,t),intersection);
    }

    /// returns true if the rect passed as parameter  intersects this one
    bool intersects(const RectI & r) const
    {
        if ( isNull() || r.isNull() ) {
            return false;
        }
        if ( (r.x2 <= x1) || (x2 <= r.x1) || (r.y2 <= y1) || (y2 <= r.y1) ) {
            return false;
        }

        return true;
    }

    bool intersects(int l,
                    int b,
                    int r,
                    int t) const
    {
        return intersects( RectI(l,b,r,t) );
    }

    /*the area : w*h*/
    U64 area() const
    {
        return (U64)width() * height();
    }

    RectI & operator=(const RectI & other)
    {
        x1 = other.left();
        y1 = other.bottom();
        x2 = other.right();
        y2 = other.top();

        return *this;
    }

    bool contains(const RectI & other) const
    {
        return other.x1 >= x1 &&
               other.y1 >= y1 &&
               other.x2 <= x2 &&
               other.y2 <= y2;
    }

    bool contains(int x,
                  int y) const
    {
        return x >= x1 && x < x2 && y >= y1 && y < y2;
    }

    bool contains(double x,
                  double y) const
    {
        return x >= x1 && x < x2 && y >= y1 && y < y2;
    }

    void translate(int dx,
                   int dy)
    {
        x1 += dx;
        y1 += dy;
        x2 += dx;
        y2 += dy;
    }

    void debug() const
    {
        std::cout << "RectI is..." << std::endl;
        std::cout << "left = " << x1 << std::endl;
        std::cout << "bottom = " << y1 << std::endl;
        std::cout << "right = " << x2 << std::endl;
        std::cout << "top = " << y2 << std::endl;
    }

    std::vector<RectI> splitIntoSmallerRects(int splitsCount) const;

    static RectI fromOfxRectI(const OfxRectI & r)
    {
        RectI ret(r.x1,r.y1,r.x2,r.y2);

        return ret;
    }
};

GCC_DIAG_ON(strict-overflow)


/// equality of boxes
inline bool
operator==(const RectI & b1,
           const RectI & b2)
{
    return b1.left() == b2.left() &&
           b1.bottom() == b2.bottom() &&
           b1.right() == b2.right() &&
           b1.top() == b2.top();
}

/// inequality of boxes
inline bool
operator!=(const RectI & b1,
           const RectI & b2)
{
    return b1.left() != b2.left() ||
           b1.bottom() != b2.bottom() ||
           b1.right() != b2.right() ||
           b1.top() != b2.top();
}

BOOST_SERIALIZATION_ASSUME_ABSTRACT(RectI);

Q_DECLARE_METATYPE(RectI)

class RectD
{
public:

    double x1; // left
    double y1; // bottom
    double x2; // right
    double y2; // top

    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar,
                   const unsigned int version)
    {
        Q_UNUSED(version);
        ar & boost::serialization::make_nvp("Left",x1);
        ar & boost::serialization::make_nvp("Bottom",y1);
        ar & boost::serialization::make_nvp("Right",x2);
        ar & boost::serialization::make_nvp("Top",y2);
    }

    RectD()
        : x1(0), y1(0), x2(0), y2(0)
    {
    }

    RectD(double l,
          double b,
          double r,
          double t)
        : x1(l), y1(b), x2(r), y2(t)                                             /*assert((x2>= x1) && (y2>=y1));*/
    {
    }

    RectD(const RectD &b)
        : x1(b.x1),y1(b.y1),x2(b.x2),y2(b.y2)
    {
        assert( (x2 >= x1) && (y2 >= y1) );
    }

    virtual ~RectD()
    {
    }

    double left() const
    {
        return x1;
    }

    void set_left(double v)
    {
        x1 = v;
    }

    double bottom() const
    {
        return y1;
    }

    void set_bottom(double v)
    {
        y1 = v;
    }

    double right() const
    {
        return x2;
    }

    void set_right(double v)
    {
        x2 = v;
    }

    double top() const
    {
        return y2;
    }

    void set_top(double v)
    {
        y2 = v;
    }

    double width() const
    {
        return x2 - x1;
    }

    double height() const
    {
        return y2 - y1;
    }

    void set(double l,
             double b,
             double r,
             double t)
    {
        x1 = l;
        y1 = b;
        x2 = r;
        y2 = t;
        /*assert((x2>= x1) && (y2>=y1));*/
    }
    
    //Useful for bbox computations
    void setupInfinity()
    {
        x1 = std::numeric_limits<double>::infinity();
        x2 = -std::numeric_limits<double>::infinity();
        y1 = std::numeric_limits<double>::infinity();
        y2 = -std::numeric_limits<double>::infinity();
    }

    void set(const RectD & b)
    {
        *this = b;
    }

    bool isInfinite() const
    {
        return x1 <= kOfxFlagInfiniteMin || x2 >= kOfxFlagInfiniteMax || y1 <= kOfxFlagInfiniteMin || y2 >= kOfxFlagInfiniteMax;
    }

    /*
       // RectD are in canonical coordinates and should never be scaled!
       RectD scaled(double sx,double sy) const {
        RectD ret;
        ret.x1 = x1;
        ret.y1 = y1;
        ret.x2 = (double)x2 * sx;
        ret.y2 = (double)y2 * sy;
        return ret;
       }
     */

    bool isNull() const
    {
        return (x2 <= x1) || (y2 <= y1);
    }

    void clear()
    {
        x1 = 0;
        y1 = 0;
        x2 = 0;
        y2 = 0;
    }

    void translate(int dx,
                   int dy)
    {
        x1 += dx;
        y1 += dy;
        x2 += dx;
        y2 += dy;
    }

    /*merge the current box with another integerBox.
     * The current box is the smallest box enclosing the two boxes
       (not the union, which is not a box).*/
    void merge(const RectD & box)
    {
        merge( box.left(), box.bottom(), box.right(), box.top() );
    }

    void merge(double l,
               double b,
               double r,
               double t)
    {
        x1 = std::min(x1, l);
        x2 = std::max(x2, r);
        y1 = std::min(y1, b);
        y2 = std::max(y2, t);
    }

    /*intersection of two boxes*/
    bool intersect(const RectD & r,
                   RectD* intersection) const
    {
        if ( isNull() || r.isNull() ) {
            return false;
        }

        if ( (x1 > r.x2) || (r.x1 > x2) || (y1 > r.y2) || (r.y1 > y2) ) {
            return false;
        }

        intersection->x1 = std::max(x1,r.x1);
        intersection->x2 = std::min(x2,r.x2);
        intersection->y1 = std::max(y1,r.y1);
        intersection->y2 = std::min(y2,r.y2);

        return true;
    }

    bool intersect(double l,
                   double b,
                   double r,
                   double t,
                   RectD* intersection) const
    {
        return intersect(RectD(l,b,r,t),intersection);
    }

    /// returns true if the rect passed as parameter is intersects this one
    bool intersects(const RectD & r) const
    {
        if ( isNull() || r.isNull() ) {
            return false;
        }
        if ( (x1 > r.x2) || (r.x1 > x2) || (y1 > r.y2) || (r.y1 > y2) ) {
            return false;
        }

        return true;
    }

    bool intersects(double l,
                    double b,
                    double r,
                    double t) const
    {
        return intersects( RectD(l,b,r,t) );
    }

    /*the area : w*h*/
    double area() const
    {
        return (double)width() * height();
    }

    RectD & operator=(const RectD & other)
    {
        x1 = other.left();
        y1 = other.bottom();
        x2 = other.right();
        y2 = other.top();

        return *this;
    }

    bool contains(const RectD & other) const
    {
        return other.x1 >= x1 &&
               other.y1 >= y1 &&
               other.x2 <= x2 &&
               other.y2 <= y2;
    }

    bool contains(double x,
                  double y) const
    {
        return x >= x1 && x < x2 && y >= y1 && y < y2;
    }

    void debug() const
    {
        std::cout << "RectI is..." << std::endl;
        std::cout << "left = " << x1 << std::endl;
        std::cout << "bottom = " << y1 << std::endl;
        std::cout << "right = " << x2 << std::endl;
        std::cout << "top = " << y2 << std::endl;
    }

    void toPixelEnclosing(const RenderScale & scale,
                          double par,
                          RectI *rect) const
    {
        rect->x1 = std::floor(x1 * scale.x / par);
        rect->y1 = std::floor(y1 * scale.y);
        rect->x2 = std::ceil(x2 * scale.x / par);
        rect->y2 = std::ceil(y2 * scale.y);
    }

    void toPixelEnclosing(unsigned int mipMapLevel,
                          double par,
                          RectI *rect) const
    {
        double scale = 1. / (1 << mipMapLevel);

        rect->x1 = std::floor(x1 * scale / par);
        rect->y1 = std::floor(y1 * scale);
        rect->x2 = std::ceil(x2 * scale / par);
        rect->y2 = std::ceil(y2 * scale);
    }

    static void ofxRectDToRectD(const OfxRectD & r,
                                RectD *ret)
    {
        ret->x1 = r.x1;
        ret->x2 = r.x2;
        ret->y1 = r.y1;
        ret->y2 = r.y2;
    }
};

/// equality of boxes
inline bool
operator==(const RectD & b1,
           const RectD & b2)
{
    return b1.left() == b2.left() &&
           b1.bottom() == b2.bottom() &&
           b1.right() == b2.right() &&
           b1.top() == b2.top();
}

/// inequality of boxes
inline bool
operator!=(const RectD & b1,
           const RectD & b2)
{
    return b1.left() != b2.left() ||
           b1.bottom() != b2.bottom() ||
           b1.right() != b2.right() ||
           b1.top() != b2.top();
}

inline void
RectI::toCanonical(unsigned int thisLevel,
                   double par,
                   const RectD & rod,
                   RectD *rect) const
{
    toCanonical_noClipping(thisLevel, par, rect);
    rect->intersect(rod, rect);
}

void
RectI::toCanonical_noClipping(unsigned int thisLevel,
                              double par,
                              RectD *rect) const
{
    rect->x1 = (x1 << thisLevel) * par;
    rect->x2 = (x2 << thisLevel) * par;
    rect->y1 = y1 << thisLevel;
    rect->y2 = y2 << thisLevel;
}

BOOST_SERIALIZATION_ASSUME_ABSTRACT(RectD);

Q_DECLARE_METATYPE(RectD)


#endif // NATRON_ENGINE_RECT_H_
