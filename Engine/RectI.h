/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2023 The Natron developers
 * (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
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

#ifndef Engine_RectI_h
#define Engine_RectI_h

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <cassert>
#include <iostream>
#include <vector>
#include <utility>
#include <cmath>
#include <algorithm> // min, max
#include <limits>

#include "Global/GlobalDefines.h"

#include "Engine/EngineFwd.h"

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
//Shiboken fails if defined at the start of a header
// clang-format off
GCC_DIAG_OFF(strict-overflow)
// clang-format on
#endif

NATRON_NAMESPACE_ENTER;

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

    RectI(int x1_,
          int y1_,
          int x2_,
          int y2_)
        : x1(x1_), y1(y1_), x2(x2_), y2(y2_)
    {
        assert( (x2 >= x1) && (y2 >= y1) );
    }

    RectI(const RectI &b)
        : x1(b.x1), y1(b.y1), x2(b.x2), y2(b.y2)
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

    void set(int x1_,
             int y1_,
             int x2_,
             int y2_)
    {
        x1 = x1_;
        y1 = y1_;
        x2 = x2_;
        y2 = y2_;
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

    /**
     * @brief Converts this rectangle to canonical coordinates and clips it if it intersects with the rod provided.
     */
    RectD toCanonical(unsigned int thisLevel, double par, const RectD & rod) const;

    RectD toCanonical_noClipping(unsigned int thisLevel, double par) const;

    /**
     * @brief Converts this rectangle to an equivalent one at a different mipmap level.
     *
     * @param fromLevel The mipmap level of this rectangle.
     * @param toLevel The mipmap level we want to convert to.
     * @param par The pixel aspect ratio for this rectangle.
     * @param rod The region of definition to clip this rectangle to when converting it to canonical coordinates.
     *
     **/
    RectI toNewMipmapLevel(unsigned int fromLevel, unsigned int toLevel, double par, const RectD& rod) const;

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
        merge(box.x1, box.y1, box.x2, box.y2);
    }

    void merge(int x1_,
               int y1_,
               int x2_,
               int y2_)
    {
        if ( isNull() ) {
            x1 = x1_;
            y1 = y1_;
            x2 = x2_;
            y2 = y2_;
        } else {
            x1 = std::min(x1, x1_);
            y1 = std::min(y1, y1_);
            x2 = std::max(x2, x2_);
            y2 = std::max(y2, y2_);
        }
    }


private:
    /**
     * @brief Computes intersection of this rectangle and the one provided as a parameter, if they overlap.
     *
     * @param[in] r The rectangle to intersect with this rectangle.
     * @param[out] intersection Updated to contain the intersection of the two rectangles if this method returns true. Otherwise left unmodified.
     * @returns True if this rectangle and the one passed in are not null and overlap.
     **/
    // Note: This is private to encourage the use of intersect() or clipIfOverlaps().
    bool intersectInternal(const RectI & r,
                   RectI* intersection) const
    {
        if ( !intersects(r) ) {
            return false;
        }

        intersection->x1 = std::max(x1, r.x1);
        intersection->y1 = std::max(y1, r.y1);
        // the region must be *at least* empty, thus the maximin.
        intersection->x2 = std::max( intersection->x1, std::min(x2, r.x2) );
        // the region must be *at least* empty, thus the maximin.
        intersection->y2 = std::max( intersection->y1, std::min(y2, r.y2) );

        assert( !intersection->isNull() );

        return true;
    }

public:

    /**
     * @brief Computes the intersection of this rectangle and rect.
     *
     * @returns Intersection of the two rectangles if they overlap. If there is no overlap a null (i.e. default constructed) rectangle is returned.
     **/
    RectI intersect(const RectI & rect) const
    {
        RectI intersection;
        intersectInternal(rect, &intersection);
        return intersection;
    }

    RectI intersect(int x1_,
                    int y1_,
                    int x2_,
                    int y2_) const
    {
        return intersect(RectI(x1_, y1_, x2_, y2_));
    }

    /**
     * @brief Updates this rectangle to the intersection rectangle if this rectangle overlaps with rect. If there is no overlap then this rectangle remains unchanged.
     *
     * @returns True if the rectangles overlap and this rectangle was clipped.
     **/
    bool clipIfOverlaps(const RectI& rect)
    {
        return intersectInternal(rect, this);
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

    bool intersects(int x1_,
                    int y1_,
                    int x2_,
                    int y2_) const
    {
        return intersects( RectI(x1_, y1_, x2_, y2_) );
    }

    /*the area : w*h*/
    U64 area() const
    {
        return (U64)width() * height();
    }

    RectI & operator=(const RectI & other)
    {
        x1 = other.x1;
        y1 = other.y1;
        x2 = other.x2;
        y2 = other.y2;

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

#ifdef DEBUG
    void debug() const
    {
        std::cout << "x1 = " << x1 << " y1 = " << y1 << " x2 = " << x2 << " y2 = " << y2 << std::endl;
    }

#endif
    std::vector<RectI> splitIntoSmallerRects(int splitsCount) const;
    static RectI fromOfxRectI(const OfxRectI & r)
    {
        RectI ret(r.x1, r.y1, r.x2, r.y2);

        return ret;
    }
};

GCC_DIAG_ON(strict-overflow)


/// equality of boxes
inline bool
operator==(const RectI & b1,
           const RectI & b2)
{
    return (b1.x1 == b2.x1 &&
            b1.y1 == b2.y1 &&
            b1.x2 == b2.x2 &&
            b1.y2 == b2.y2);
}

/// inequality of boxes
inline bool
operator!=(const RectI & b1,
           const RectI & b2)
{
    return !(b1 == b2);
}

NATRON_NAMESPACE_EXIT;

Q_DECLARE_METATYPE(NATRON_NAMESPACE::RectI)

#endif // Engine_RectI_h
