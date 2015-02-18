//  Natron
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "Rect.h"

#define MINAREA 4096 // minimum rectangle area = 4096 pixels (=64*64)

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
    // make sure there are at least 4096 pixels (=64*64) per rect, and the rects are as square as possible.
    // This minimizes the overlapping areas between rendered regions
    if (area() <= MINAREA) {
        ret.push_back(*this);
    } else {
        // the average rect area
        double avgArea = std::max((double)MINAREA, area() / (double)splitsCount);
        int numCols = std::max(1, (int)(width() / std::sqrt(avgArea)));
        int numRows = std::max(1, std::min(splitsCount / numCols, (int)(area()/(MINAREA*numCols)))); // integer division
        numCols = std::max(1, std::min(splitsCount / numRows, (int)(area()/(MINAREA*numRows))));
        assert(splitsCount >= numRows * numCols);
        for (int i = 0; i < numRows; ++i) {
            int y1 = bottom() + i     * height() / numRows;
            int y2 = bottom() + (i+1) * height() / numRows;
            for (int j = 0; j < numCols; ++j) {
                int x1 = left() + j     * width() / numCols;
                int x2 = left() + (j+1) * width() / numCols;
                //assert((x2-x1)*(y2-y1) >= MINAREA);
                //printf("area is %d\n", (x2-x1)*(y2-y1));
                ret.push_back( RectI(x1, y1, x2, y2) );
            }
        }
    }
#endif

    return ret;
}
