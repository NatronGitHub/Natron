/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2016 INRIA and Alexandre Gauthier-Foichat
 *
 * Natron is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Natron is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Natron.  If not, see <http://www.gnu.org/licenses/gpl-2.0.html>
 * ***** END LICENSE BLOCK ***** */

#ifndef Engine_RectD_h
#define Engine_RectD_h

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include <cassert>
#include <iostream>
#include <vector>
#include <utility>
#include <cmath>
#include <algorithm> // min, max
#include <limits>

#include "Global/GlobalDefines.h"
#include "Global/Macros.h"

#include "Engine/EngineFwd.h"

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
//Shiboken fails if defined at the start of a header
GCC_DIAG_OFF(strict-overflow)
#endif

class RectD
{
public:

    double x1; // left
    double y1; // bottom
    double x2; // right
    double y2; // top

    template<class Archive>
    void serialize(Archive & ar,
                   const unsigned int version);

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

#ifdef DEBUG
    void debug() const
    {
        std::cout << "x1 = "<<x1<<" y1 = "<<y1<<" x2 = "<<x2<<" y2 = "<<y2<< std::endl;
    }
#endif
    
    void toPixelEnclosing(const RenderScale & scale,
                          double par,
                          RectI *rect) const;

    void toPixelEnclosing(unsigned int mipMapLevel,
                          double par,
                          RectI *rect) const;

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

Q_DECLARE_METATYPE(RectD)


#endif // Engine_RectD_h
