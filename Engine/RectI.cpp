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

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "RectI.h"

#include <algorithm> // min, max
#include <cassert>
#include <stdexcept>

#include "Engine/RectD.h"

#define MINAREA64 4096  // = 4096 pixels (=64*64)
#define MINAREA128 16384
#define MINAREA256 65536
#define MINAREA MINAREA128 // minimum rectangle area

NATRON_NAMESPACE_ENTER;

/// if splitCount is zero, this function returns a set of less than area()/MINAREA rects which are no smaller than MINAREA
std::vector<RectI> RectI::splitIntoSmallerRects(int splitsCount) const
{
    std::vector<RectI> ret;

    if ( isNull() ) {
        return ret;
    }
#ifdef NATRON_SPLITRECT_SCANLINE
    int averagePixelsPerSplit = std::ceil(double( area() ) / (double)splitsCount);
    /*if the splits happen to have less pixels than 1 scan-line contains, just do scan-line rendering*/
    if ( averagePixelsPerSplit < width() ) {
        for (int i = bottom(); i < top(); ++i) {
            ret.push_back( RectI(left(), i, right(), i + 1) );
        }
    } else {
        //we round to the ceil
        int scanLinesCount = std::ceil( (double)averagePixelsPerSplit / (double)width() );
        int startBox = bottom();
        while (startBox < top() - scanLinesCount) {
            ret.push_back( RectI(left(), startBox, right(), startBox + scanLinesCount) );
            startBox += scanLinesCount;
        }
        if ( startBox < top() ) {
            ret.push_back( RectI( left(), startBox, right(), top() ) );
        }
    }
#else
    // make sure there are at least MINAREA pixels (=128*128) per rect, and the rects are as square as possible.
    // This minimizes the overlapping areas between rendered regions
    if (area() <= MINAREA) {
        ret.push_back(*this);
    } else {
        // autocompute splitsCount
        if (!splitsCount) {
            splitsCount = area() / MINAREA;
        }
        //printf("splitsCount=%d\n", splitsCount);
        // the average rect area
        double avgArea = area() / (double)splitsCount;
        //printf("avgArea=%g,sqrt=%g\n", avgArea, sqrt(avgArea));
        bool landscape = width() > height();
        int dim1 = landscape ? width() : height();
        int dim2 = landscape ? height() : width();
        //printf("dim1=%d\n", dim1);
        //printf("dim2=%d\n", dim2);

        int num1 = (int)(std::ceil(dim1 / std::sqrt(avgArea)));
        assert(num1 > 0);
        //printf("num1=%d\n", num1);
        int num2 = std::max(1, std::min(splitsCount / num1, dim2/(MINAREA/(dim1/num1)))); // integer division
        assert(num1 >= num2);
        //printf("num2=%d\n", num2);
        num1 = std::max(1, std::min(splitsCount / num2, dim1/(1+(MINAREA-1)/(dim2/num2))));
        //printf("num1=%d\n", num1);
        assert(splitsCount >= num1 * num2);
        assert((dim1/num1)*(dim2/num2) >= MINAREA);
        int numRows = landscape ? num2 : num1;
        int numCols = landscape ? num1 : num2;
        for (int i = numRows - 1; i >= 0; --i) {
            int y1_ = bottom() + i     * height() / numRows;
            int y2_ = bottom() + (i+1) * height() / numRows;
            for (int j = 0; j < numCols; ++j) {
                int x1_ = left() + j     * width() / numCols;
                int x2_ = left() + (j+1) * width() / numCols;
                //printf("x1_=%d,x2_=%d,y1_=%d,y2_=%d\n",x1_,x2_,y1_,y2_);
                assert((x2_-x1_)*(y2_-y1_) >= MINAREA);
                //printf("area is %d\n", (x2_-x1_)*(y2_-y1_));
                ret.push_back( RectI(x1_, y1_, x2_, y2_) );
            }
        }
    }
#endif

    return ret;
}

void
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

NATRON_NAMESPACE_EXIT;
