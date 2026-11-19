/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2026 The Natron developers
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

#ifndef NATRON_TESTS_NANGUARDS_FASTMATH_H
#define NATRON_TESTS_NANGUARDS_FASTMATH_H

// ***** BEGIN PYTHON BLOCK *****
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

NATRON_NAMESPACE_ENTER

// MathUtils tests compiled with -ffast-math, see NaNGuards_FastMath.cpp
namespace NaNGuardsFastMath {
bool compiledWithFastMath();
bool isFiniteD(double x);
bool isFiniteF(float x);
bool isNaNF(float x);
int countNaN(const float* p, int n);
bool isSumFinite(double a, double b);
}

NATRON_NAMESPACE_EXIT

#endif // NATRON_TESTS_NANGUARDS_FASTMATH_H
