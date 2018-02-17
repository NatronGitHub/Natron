/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
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


#include "PyExprUtils.h"

#include <cmath>
#include <boost/cstdint.hpp>

#include "Global/GlobalDefines.h"
#include "Engine/Noise.h"

using boost::uint32_t;

#ifndef UINT32_MAX
#define UINT32_MAX  ((uint32_t)-1)
#endif

NATRON_NAMESPACE_ENTER
NATRON_PYTHON_NAMESPACE_ENTER

double
ExprUtils::linearstep(double x, double a, double b) {
    if (a < b) {
        return x < a ? 0 : (x > b ? 1 : (x - a) / (b - a));
    } else if (a > b) {
        return 1 - (x < b ? 0 : (x > a ? 1 : (x - b) / (a - b)));
    }
    return boxstep(x, a);
}

double
ExprUtils::smoothstep(double x, double a, double b) {
    if (a < b) {
        if (x < a) return 0;
        if (x >= b) return 1;
        x = (x - a) / (b - a);
    } else if (a > b) {
        if (x <= b) return 1;
        if (x > a) return 0;
        x = 1 - (x - b) / (a - b);
    } else
        return boxstep(x, a);
    return x * x * (3 - 2 * x);
}

double
ExprUtils::gaussstep(double x, double a, double b) {
    if (a < b) {
        if (x < a) return 0;
        if (x >= b) return 1;
        x = 1 - (x - a) / (b - a);
    } else if (a > b) {
        if (x <= b) return 1;
        if (x > a) return 0;
        x = (x - b) / (a - b);
    } else
        return boxstep(x, a);
    return pow(2, -8 * x * x);
}

double
ExprUtils::remap(double x, double source, double range, double falloff, double interp) {
    range = fabs(range);
    falloff = fabs(falloff);

    if (falloff == 0) return fabs(x - source) < range;

    double a, b;
    if (x > source) {
        a = source + range;
        b = a + falloff;
    } else {
        a = source - range;
        b = a - falloff;
    }

    switch (int(interp)) {
        case 0:
            return linearstep(x, b, a);
        case 1:
            return smoothstep(x, b, a);
        default:
            return gaussstep(x, b, a);
    }
}

double
ExprUtils::hash(const std::vector<double>& args)
{
    // combine args into a single seed
    U32 seed = 0;
    for (int i = 0; i < (int)args.size(); i++) {
        // make irrational to generate fraction and combine xor into 32 bits
        int exp = 0;
        double frac = frexp(args[i] * double(M_E * M_PI), &exp);
        U32 s = (U32)(frac * UINT32_MAX) ^ (U32)exp;

        // blend with seed (constants from Numerical Recipes, attrib. from Knuth)
        static const U32 M = 1664525, C = 1013904223;
        seed = seed * M + s + C;
    }

    // tempering (from Matsumoto)
    seed ^= (seed >> 11);
    seed ^= (seed << 7) & 0x9d2c5680UL;
    seed ^= (seed << 15) & 0xefc60000UL;
    seed ^= (seed >> 18);

    // permute
    static unsigned char p[256] = {
        148, 201, 203, 34,  85,  225, 163, 200, 174, 137, 51,  24,  19,  252, 107, 173, 110, 251, 149, 69,  180, 152,
        141, 132, 22,  20,  147, 219, 37,  46,  154, 114, 59,  49,  155, 161, 239, 77,  47,  10,  70,  227, 53,  235,
        30,  188, 143, 73,  88,  193, 214, 194, 18,  120, 176, 36,  212, 84,  211, 142, 167, 57,  153, 71,  159, 151,
        126, 115, 229, 124, 172, 101, 79,  183, 32,  38,  68,  11,  67,  109, 221, 3,   4,   61,  122, 94,  72,  117,
        12,  240, 199, 76,  118, 5,   48,  197, 128, 62,  119, 89,  14,  45,  226, 195, 80,  50,  40,  192, 60,  65,
        166, 106, 90,  215, 213, 232, 250, 207, 104, 52,  182, 29,  157, 103, 242, 97,  111, 17,  8,   175, 254, 108,
        208, 224, 191, 112, 105, 187, 43,  56,  185, 243, 196, 156, 246, 249, 184, 7,   135, 6,   158, 82,  130, 234,
        206, 255, 160, 236, 171, 230, 42,  98,  54,  74,  209, 205, 33,  177, 15,  138, 178, 44,  116, 96,  140, 253,
        233, 125, 21,  133, 136, 86,  245, 58,  23,  1,   75,  165, 92,  217, 39,  0,   218, 91,  179, 55,  238, 170,
        134, 83,  25,  189, 216, 100, 129, 150, 241, 210, 123, 99,  2,   164, 16,  220, 121, 139, 168, 64,  190, 9,
        31,  228, 95,  247, 244, 81,  102, 145, 204, 146, 26,  87,  113, 198, 181, 127, 237, 169, 28,  93,  27,  41,
        231, 248, 78,  162, 13,  186, 63,  66,  131, 202, 35,  144, 222, 223};
    union {
        uint32_t i;
        unsigned char c[4];
    } u1, u2;
    u1.i = seed;
    u2.c[3] = p[u1.c[0]];
    u2.c[2] = p[(u1.c[1] + u2.c[3]) & 0xff];
    u2.c[1] = p[(u1.c[2] + u2.c[2]) & 0xff];
    u2.c[0] = p[(u1.c[3] + u2.c[1]) & 0xff];

    // scale to [0.0 .. 1.0]
    return u2.i * (1.0 / UINT32_MAX);
}

double
ExprUtils::noise(double x)
{
    double ret = 0.;
    // coverity[callee_ptr_arith]
    Noise<1, 1>(&x, &ret);
    return ret;
}

double
ExprUtils::noise(const Double2DTuple& p)
{
    double ret = 0.;
    // coverity[callee_ptr_arith]
    Noise<2, 1>((const double*)&p.x, &ret);
    return ret;
}

double
ExprUtils::noise(const Double3DTuple& p)
{
    double ret = 0.;
    // coverity[callee_ptr_arith]
    Noise<3, 1>((const double*)&p.x, &ret);
    return ret;
}

double
ExprUtils::noise(const ColorTuple& p)
{
    double ret = 0.;
    // coverity[callee_ptr_arith]
    Noise<4, 1>((const double*)&p.r, &ret);
    return ret;
}


double
ExprUtils::snoise(const Double3DTuple& p)
{
    double result = 0.;
    // coverity[callee_ptr_arith]
    Noise<3, 1>((const double*)&p.x, &result);
    return result;

}

Double3DTuple
ExprUtils::vnoise(const Double3DTuple& p)
{
    Double3DTuple result = {0., 0., 0.};
    Noise<3, 3>((const double*)&p.x, (double*)&result.x);
    return result;

}

Double3DTuple
ExprUtils::cnoise(const Double3DTuple& p)
{
    Double3DTuple ret = {0., 0., 0.};
    Noise<3, 3>((const double*)&p.x, (double*)&ret.x);
    ret.x = p.x * 0.5 + 0.5;
    ret.y = p.y * 0.5 + 0.5;
    ret.z = p.z * 0.5 + 0.5;
    return ret;
}

double
ExprUtils::snoise4(const ColorTuple& p)
{
    double result = 0.;
    // coverity[callee_ptr_arith]
    Noise<4, 1>((const double*)&p.r, &result);
    return result;
}

Double3DTuple
ExprUtils::vnoise4(const ColorTuple& p)
{
    Double3DTuple result = {0., 0., 0.};
    Noise<4, 3>((const double*)&p.r, (double*)&result.x);
    return result;
}

Double3DTuple
ExprUtils::cnoise4(const ColorTuple& p)
{
    Double3DTuple result = {0., 0., 0.};
    Noise<4, 3>((const double*)&p.r, (double*)&result.x);
    result.x = result.x * 0.5 + 0.5;
    result.y = result.y * 0.5 + 0.5;
    result.z = result.z * 0.5 + 0.5;
    return result;
}

double
ExprUtils::turbulence(const Double3DTuple& p, int octaves, double lacunarity, double gain)
{
    octaves = std::min(std::max(octaves, 1), 8);
    double result = 0.;
    // coverity[callee_ptr_arith]
    FBM<3, 1, true>((const double*)&p.x, &result, octaves, lacunarity, gain);
    return .5 * result + .5;
}

Double3DTuple
ExprUtils::vturbulence(const Double3DTuple& p, int octaves, double lacunarity, double gain)
{
    octaves = std::min(std::max(octaves, 1), 8);
    Double3DTuple result = {0., 0., 0.};
    FBM<3, 3, true>((const double*)&p.x, (double*)&result.x, octaves, lacunarity, gain);
    return result;

}

Double3DTuple
ExprUtils::cturbulence(const Double3DTuple& p, int octaves, double lacunarity, double gain)
{
    octaves = std::min(std::max(octaves, 1), 8);
    Double3DTuple result = {0., 0., 0.};
    FBM<3, 3, true>((const double*)&p.x, (double*)&result.x, octaves, lacunarity, gain);
    result.x = result.x * 0.5 + 0.5;
    result.y = result.y * 0.5 + 0.5;
    result.z = result.z * 0.5 + 0.5;
    return result;
}

double
ExprUtils::fbm(const Double3DTuple& p, int octaves, double lacunarity, double gain)
{
    octaves = std::min(std::max(octaves, 1), 8);
    double result = 0.0;
    // coverity[callee_ptr_arith]
    FBM<3, 1, false>((const double*)&p.x, &result, octaves, lacunarity, gain);
    return .5 * result + .5;

}

Double3DTuple
ExprUtils::vfbm(const Double3DTuple& p, int octaves, double lacunarity, double gain)
{
    octaves = std::min(std::max(octaves, 1), 8);
    Double3DTuple result = {0., 0., 0.};
    FBM<3, 3, false>((const double*)&p.x, (double*)&result.x, octaves, lacunarity, gain);
    return result;
}

double
ExprUtils::fbm4(const ColorTuple& p, int octaves, double lacunarity, double gain)
{
    octaves = std::min(std::max(octaves, 1), 8);
    double result = 0.0;
    // coverity[callee_ptr_arith]
    FBM<4, 1, false>((const double*)&p.r, &result, octaves, lacunarity, gain);
    return .5 * result + .5;
}

Double3DTuple
ExprUtils::vfbm4(const ColorTuple& p, int octaves, double lacunarity, double gain)
{
    octaves = std::min(std::max(octaves, 1), 8);
    Double3DTuple result = {0., 0., 0.};
    FBM<4, 3, false>((const double*)&p.r, (double*)&result.x, octaves, lacunarity, gain);
    return result;
}

Double3DTuple
ExprUtils::cfbm(const Double3DTuple& p, int octaves, double lacunarity, double gain)
{
    octaves = std::min(std::max(octaves, 1), 8);
    Double3DTuple result = {0., 0., 0.};
    FBM<3, 3, false>((const double*)&p.x, (double*)&result.x, octaves, lacunarity, gain);
    result.x = result.x * 0.5 + 0.5;
    result.y = result.y * 0.5 + 0.5;
    result.z = result.z * 0.5 + 0.5;
    return result;

}

Double3DTuple
ExprUtils::cfbm4(const ColorTuple& p, int octaves, double lacunarity, double gain)
{
    octaves = std::min(std::max(octaves, 1), 8);
    Double3DTuple result = {0., 0., 0.};
    FBM<4, 3, false>((const double*)&p.r, (double*)&result.x, octaves, lacunarity, gain);
    result.x = result.x * 0.5 + 0.5;
    result.y = result.y * 0.5 + 0.5;
    result.z = result.z * 0.5 + 0.5;
    return result;

}

double
ExprUtils::cellnoise(const Double3DTuple& p)
{
    double result = 0.;
    // coverity[callee_ptr_arith]
    CellNoise<3, 1>((const double*)&p.x, &result);
    return result;
}

Double3DTuple
ExprUtils::ccellnoise(const Double3DTuple& p)
{
    Double3DTuple result = {0., 0., 0.};
    CellNoise<3, 1>((const double*)&p.x, (double*)&result.x);
    return result;
}

double
ExprUtils::pnoise(const Double3DTuple& p, const Double3DTuple& period)
{
    double result = 0.;
    int pargs[3] = {std::max(1,(int)period.x),
        std::max(1,(int)period.y),
        std::max(1,(int)period.z)};
    // coverity[callee_ptr_arith]
    PNoise<3, 1>((const double*)&p.x, pargs, &result);
    return result;

}

NATRON_PYTHON_NAMESPACE_EXIT
NATRON_NAMESPACE_EXIT
