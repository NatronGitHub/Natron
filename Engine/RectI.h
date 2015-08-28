/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2015 INRIA and Alexandre Gauthier-Foichat
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

#ifndef _Engine_RectI_h_
#define _Engine_RectI_h_

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

    template<class Archive>
    void serialize(Archive & ar,
                   const unsigned int version);

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

    void toCanonical(unsigned int thisLevel, double par, const RectD & rod, RectD *rect) const;
    void toCanonical_noClipping(unsigned int thisLevel, double par, RectD *rect) const;

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

Q_DECLARE_METATYPE(RectI)

#endif // _Engine_RectI_h_
