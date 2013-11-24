/*
 * TICKS algorithms
 * Author: Frederic Devernay
 * The goal of this algorithm is to compute the position of ticks on an axis.
 * the ticks are positioned at multiples of 1 and 5 of powers of 10
 */

#ifndef NATRON_GUI_TICKS_H_
#define NATRON_GUI_TICKS_H_

#include <vector>

// tick_size_10
//
// for a range xmin,xmax drawn with a size range_units (in pixels, or cm...),
// find the smallest tick size, which is a multiple of a power of ten,
// and is drawn larger than min_tick_size_units (in pixels, or cm...).
double ticks_size_10(double xmin, double xmax, double range_units, double min_tick_size_units);
 
// tick_size
//
// for a range xmin,xmax drawn with a size range_units (in pixels, or cm...),
// find the smallest tick size, which is a multiple of a power of ten or of five times a
// power of ten, and is drawn larger than min_tick_size_units (in pixels, or cm...).
// power of ten, and is drawn larger than min_tick_size_units (in pixels, or cm...).
void ticks_size(double xmin, double xmax, double range_units, double min_tick_size_units, double *t_size, bool* half_tick);

// ticks_bounds
//
// Find the index of the first and last tick of width tick_width within the interval xmin,xmax.
// if tick_max != 0, an offset is computed to avoid overflow on m1 and m2
// (e.g. if xmin=1+1e-10 and xmax = 1+2e-10).
// This offset is computed so that the maximum tick value within the interval is below tick_max.
// The tick value represents the "majorness" of a tick (tick values are 1, 5, 10, 50, etc.).
// tick_max should be a power of 10. A good value for tick_max is 1000. tick_max should be a multiple of 50
void ticks_bounds(double xmin, double xmax, double tick_width, bool half_tick, int tick_max, double *offset, int* m1, int* m2);

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
ticks_fill(bool half_tick, int tick_max, int m1, int m2, std::vector<int>* ticks);

#endif
