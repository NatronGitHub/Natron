/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2021 The Natron developers
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

#ifndef PYNOISE_H
#define PYNOISE_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include <vector>

#include "Global/Macros.h"

#include "Engine/PyParameter.h"

NATRON_NAMESPACE_ENTER;
NATRON_PYTHON_NAMESPACE_ENTER;


class ExprUtils
{
public:

    ExprUtils()
    {

    }

    // if x < a then 0 otherwise 1
    static double boxstep(double x, double a) { return x < a ? 0.0 : 1.0; }

    // Transitions linearly when a < x < b
    static double linearstep(double x, double a, double b);

    // Transitions smoothly (cubic) when a < x < b
    static double smoothstep(double x, double a, double b);

    // Transitions smoothly (exponentially) when a < x < b
    static double gaussstep(double x, double a, double b);

    // General remapping function.
    // When x is within +/- range of source, the result is one.
    // The result falls to zero beyond that range over falloff distance.
    // The falloff shape is controlled by interp.
    // linear = 0
    // smooth = 1
    // gaussian = 2
    static double remap(double x, double source, double range, double falloff, double interp);

    // Linear interpolation of a and b according to alpha
    static double mix(double x, double y, double alpha) { return x * (1 - alpha) + y * alpha; }

    // Like rand, but with no internal seeds. Any number of seeds may be given
    // and the result will be a random function based on all the seeds.
    static double hash(const std::vector<double>& args);

    // Original perlin noise at location (C2 interpolant)
    static double noise(double x);
    static double noise(const Double2DTuple& p);
    static double noise(const Double3DTuple& p);
    static double noise(const ColorTuple& p);

    // signed noise w/ range -1 to 1 formed with original perlin noise at location (C2 interpolant)
    double snoise(const Double3DTuple& p);

    // vector noise formed with original perlin noise at location (C2 interpolant)
    static Double3DTuple vnoise(const Double3DTuple& p);

    // color noise formed with original perlin noise at location (C2 interpolant)
    static Double3DTuple cnoise(const Double3DTuple& p);

    // 4D signed noise w/ range -1 to 1 formed with original perlin noise at location (C2 interpolant)
    static double snoise4(const ColorTuple& p);

    // 4D vector noise formed with original perlin noise at location (C2 interpolant)
    static Double3DTuple vnoise4(const ColorTuple& p);

    // 4D color noise formed with original perlin noise at location (C2 interpolant)"
    static Double3DTuple cnoise4(const ColorTuple& p);

    // fbm (Fractal Brownian Motion) is a multi-frequency noise function.
    // The base frequency is the same as the noise function. The total
    // number of frequencies is controlled by octaves. The lacunarity is the
    // spacing between the frequencies - a value of 2 means each octave is
    // twice the previous frequency. The gain controls how much each
    // frequency is scaled relative to the previous frequency.
    static double turbulence(const Double3DTuple& p, int octaves = 6, double lacunarity = 2., double gain = 0.5);
    static Double3DTuple vturbulence(const Double3DTuple& p, int octaves = 6, double lacunarity = 2., double gain = 0.5);
    static Double3DTuple cturbulence(const Double3DTuple& p, int octaves = 6, double lacunarity = 2., double gain = 0.5);
    static double fbm(const Double3DTuple& p, int octaves = 6, double lacunarity = 2., double gain = 0.5);
    static Double3DTuple vfbm(const Double3DTuple& p, int octaves = 6, double lacunarity = 2., double gain = 0.5);
    static double fbm4(const ColorTuple& p, int octaves = 6, double lacunarity = 2., double gain = 0.5);
    static Double3DTuple vfbm4(const ColorTuple& p, int octaves = 6, double lacunarity = 2., double gain = 0.5);
    static Double3DTuple cfbm(const Double3DTuple& p, int octaves = 6, double lacunarity = 2., double gain = 0.5);
    static Double3DTuple cfbm4(const ColorTuple& p, int octaves = 6, double lacunarity = 2., double gain = 0.5);

    // cellnoise generates a field of constant colored cubes based on the integer location
    // This is the same as the prman cellnoise function
    static double cellnoise(const Double3DTuple& p);
    static Double3DTuple ccellnoise(const Double3DTuple& p);

    // periodic noise
    static double pnoise(const Double3DTuple& p, const Double3DTuple& period);
};

NATRON_PYTHON_NAMESPACE_EXIT;
NATRON_NAMESPACE_EXIT;


#endif // PYNOISE_H
