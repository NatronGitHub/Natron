/*
 * TICKS algorithms
 * Author: Frederic Devernay
 * The goal of this algorithm is to compute the position of ticks on an axis.
 * the ticks are positioned at multiples of 1 and 5 of powers of 10
 */
#include "ticks.h"
#include <cmath>

// tick_size_10
//
// for a range xmin,xmax drawn with a size range_units (in pixels, or cm...),
// find the smallest tick size, which is a multiple of a power of ten,
// and is drawn larger than min_tick_size_units (in pixels, or cm...).
double ticks_size_10(double xmin, double xmax, double range_units, double min_tick_size_units)
{
    // first, compute the size of min_tick_size_units
    double min_tick_size = min_tick_size_units * (xmax - xmin) / range_units;

    int next_p10 = std::ceil(std::log10(min_tick_size));
    double tick_size = std::pow(10.,next_p10);
    assert(tick_size/10 < min_tick_size);
    assert(tick_size >= min_tick_size);
    return tick_size;
}
 
// tick_size
//
// for a range xmin,xmax drawn with a size range_units (in pixels, or cm...),
// find the smallest tick size, which is a multiple of a power of ten or of five times a
// power of ten, and is drawn larger than min_tick_size_units (in pixels, or cm...).
double ticks_size(double xmin, double xmax, double range_units, double min_tick_size_units)
{
    // first, compute the size of min_tick_size_units
    double min_tick_size = min_tick_size_units * (xmax - xmin) / range_units;

    int next_p10 = std::ceil(std::log10(min_tick_size));
    double tick_size = std::pow(10.,next_p10);
    assert(tick_size/10 < min_tick_size);
    assert(tick_size >= min_tick_size);
    if (tick_size/2 >= min_tick_size) {
        return tick_size/2;
    }
    return tick_size;
}
 
// ticks_bounds
//
// Find the index of the first and last tick of width tick_width within the interval xmin,xmax.
// if tick_max != 0, an offset is computed to avoid overflow on m1 and m2
// (e.g. if xmin=1+1e-10 and xmax = 1+2e-10).
// This offset is computed so that the maximum tick value within the interval is below tick_max.
// The tick value represents the "majorness" of a tick (tick values are 1, 5, 10, 50, etc.).
// tick_max should be a power of 10. A good value for tick_max is 1000. tick_max should be a multiple of 50
double ticks_bounds(double xmin, double xmax, double tick_width, int tick_max, int* m1, int* m2)
{
    double offset = 0.;
    assert(tick_max >= 0);
    if (tick_max > 0 && (xmin > 0 || xmax < 0)) {
        // make sure offset is outside of the range (xmin,xmax)
        offset = (xmin > 0) ? std::floor(xmin/(tick_width*tick_max)) : std::ceil(xmax/(tick_width*tick_max));
    }
    *m1 = std::ceil((xmin-offset)/tick_width);
    *m2 = std::floor((xmax-offset)/tick_width);
    return offset;
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
ticks_fill(int tick_max, int m1, int m2, bool half_tick, std::vector<int>* ticks)
{
    ticks->resize(m2-m1+1);

    // the largest possible  tick number is the largest power of 10 below max(abs(m1),abs(m2)),
    // (or half of this, or five times less if half_tick is true)
    int m_max = std::max(std::abs(m1),std::abs(m2));
    int tick_size_max = (int)std::pow(10.,std::floor(std::log10(m_max)));
    if (half_tick) {
        // find the closest real power of 10 (i.e. 2, 20, or 200 ticks)
        if (tick_size_max / 5 >= m_max) {
            tick_size_max /= 5;
        } else {
            tick_size_max *= 2;
        }
    }
    tick_size_max = std::min(tick_size_max,tick_max);
    // now all ticks can be obtained by dividing tick_largest by 2, then 5, then 2, then 5, etc.
    int tick_size = 1;
    bool multiply_by_two = half_tick;
    while (tick_size <= tick_size_max) {
        int tick_min = std::ceil(m1/(double)tick_size) * tick_size;
        int tick_max = std::floor(m2/(double)tick_size) * tick_size;
        for (int tick = tick_min; tick <= tick_max; tick += tick_size) {
            assert(tick-m1 >= 0 && tick-m1 < (int)ticks->size());
            (*ticks)[tick-m1] = tick_size;
        }
        if (multiply_by_two) {
            tick_size *= 2;
        } else {
            tick_size *= 5;
        }
        multiply_by_two = !multiply_by_two;
    }

    // last, if 0 is within the interval, give it the value tick_max*10
    if (m1 <=0 && m2 >= 0) {
        (*ticks)[-m1] = 10 * tick_max;
    }
}
