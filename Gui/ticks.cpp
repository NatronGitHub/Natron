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

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "ticks.h"

/*
 * TICKS algorithms
 * Author: Frederic Devernay
 * The goal of this algorithm is to compute the position of ticks on an axis.
 * the ticks are positioned at multiples of 1 and 5 of powers of 10
 */

#include <cmath>
#include <cassert>
#include <algorithm> // min, max

// tick_size_10
//
// for a range xmin,xmax drawn with a size range_units (in pixels, or cm...),
// find the smallest tick size, which is a multiple of a power of ten,
// and is drawn larger than min_tick_size_units (in pixels, or cm...).
double
ticks_size_10(double xmin,
              double xmax,
              double range_units,
              double min_tick_size_units)
{
    // first, compute the size of min_tick_size_units
    double min_tick_size = min_tick_size_units * (xmax - xmin) / range_units;
    int next_p10 = std::ceil( std::log10(min_tick_size) );
    double tick_size = std::pow(10.,next_p10);

    assert(tick_size / 10 < min_tick_size);
    assert(tick_size >= min_tick_size);

    return tick_size;
}

// tick_size
//
// for a range xmin,xmax drawn with a size range_units (in pixels, or cm...),
// find the smallest tick size, which is a multiple of a power of ten or of five times a
// power of ten, and is drawn larger than min_tick_size_units (in pixels, or cm...).
void
ticks_size(double xmin,
           double xmax,
           double range_units,
           double min_tick_size_units,
           double *t_size,
           bool* half_tick)
{
    // first, compute the size of min_tick_size_units
    double min_tick_size = min_tick_size_units * (xmax - xmin) / range_units;
    int next_p10 = std::ceil( std::log10(min_tick_size) );

    *t_size = std::pow(10.,next_p10);
    assert(*t_size / 10 < min_tick_size);
    assert(*t_size >= min_tick_size);
    if (*t_size / 2 >= min_tick_size) {
        *t_size /= 2;
        if (half_tick) {
            *half_tick = true;
        }
    } else if (half_tick) {
        *half_tick = false;
    }
}

// ticks_bounds
//
// Find the index of the first and last tick of width tick_width within the interval xmin,xmax.
// if tick_max != 0, an offset is computed to avoid overflow on m1 and m2
// (e.g. if xmin=1+1e-10 and xmax = 1+2e-10).
// This offset is computed so that the maximum tick value within the interval is below tick_max.
// The tick value represents the "majorness" of a tick (tick values are 1, 5, 10, 50, etc.).
// tick_max should be a power of 10. A good value for tick_max is 1000. tick_max should be a multiple of 50
void
ticks_bounds(double xmin,
             double xmax,
             double tick_width,
             bool half_tick,
             int tick_max,
             double *offset,
             int* m1,
             int* m2)
{
    *offset = 0.;
    assert(tick_max >= 0);
    if ( (tick_max > 0) && ( (xmin > 0) || (xmax < 0) ) ) {
        const int h = half_tick ? 2 : 1;
        const int mult = h * tick_width * tick_max;
        // make sure offset is outside of the range (xmin,xmax)
        if (mult != 0) {
            *offset = mult * ( (xmin > 0) ? std::floor(xmin / mult) : std::ceil(xmax / mult) );
        } else {
            *offset = 0;
        }
    }
    *m1 = std::ceil( (xmin - *offset) / tick_width );
    *m2 = std::floor( (xmax - *offset) / tick_width );
}

// ticks_fill
//
// fill a vector of ticks.
// the first element in the vector corresponds to m1, the last to m2.
// each value stored in the vector contains the tick size at this index, which can be
// 1 (minor tick), 5 (regular tick), 10 (major tick), 50, 100, etc.
//
// If parameter half_tick is true, this means that the minor tick is actually a half tick,
// i.e. five times a power of ten, and the tick sizes are
// 1 (half minor tick), 2 (minor tick), 10 (regular tick), 20, 100, etc.
//
// tick_max is the maximum tick value that may be used, see tick_bounds().
void
ticks_fill(bool half_tick,
           int tick_max,
           int m1,
           int m2,
           std::vector<int>* ticks)
{
    ticks->resize(m2 - m1 + 1);

    // now all ticks can be obtained by dividing tick_largest by 2, then 5, then 2, then 5, etc.
    int tick_size = 1;
    bool multiply_by_two = half_tick;
    while (tick_size <= tick_max) {
        int tick_1 = std::ceil(m1 / (double)tick_size) * tick_size;
        int tick_2 = std::floor(m2 / (double)tick_size) * tick_size;
        for (int tick = tick_1; tick <= tick_2; tick += tick_size) {
            assert( tick - m1 >= 0 && tick - m1 < (int)ticks->size() );
            (*ticks)[tick - m1] = tick_size;
        }
        if (multiply_by_two) {
            tick_size *= 2;
        } else {
            tick_size *= 5;
        }
        multiply_by_two = !multiply_by_two;
    }

    // last, if 0 is within the interval, give it the value tick_max*10
    if ( (m1 <= 0) && (m2 >= 0) ) {
        (*ticks)[-m1] = 10 * tick_max;
    }
}

// compute alpha value for drawing the ticks
double
ticks_alpha(double min,
            double max,
            double val)
{
    assert(val > 0. && min > 0. && max > 0. && max > min);
    const double alpha = sqrt( (val - min) / (max - min) );

    return std::max( 0.,std::min(alpha,1.) );
}

