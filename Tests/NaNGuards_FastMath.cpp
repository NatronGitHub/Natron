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

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

// This file is compiled with -ffast-math (see Tests/CMakeLists.txt), and its functions
// are called from NaNGuards_Test.cpp with NaN and infinite values computed at run time,
// so that the compiler can't see where they come from. With -ffast-math, clang marks
// the arguments as never NaN or infinite, and without the optimization barrier in
// MathUtils::floatBits() it folds these tests to constants.

#include "NaNGuards_FastMath.h"

#include "Global/MathUtils.h"

#if defined(NATRON_TESTS_FAST_MATH_TU) && defined(__GNUC__) && !defined(__FAST_MATH__)
#error "NaNGuards_FastMath.cpp must be compiled with -ffast-math"
#endif

NATRON_NAMESPACE_ENTER

namespace NaNGuardsFastMath {

bool
compiledWithFastMath()
{
#ifdef __FAST_MATH__
    return true;
#else
    return false;
#endif
}

bool
isFiniteD(double x)
{
    return MathUtils::isFinite(x);
}

bool
isFiniteF(float x)
{
    return MathUtils::isFinite(x);
}

bool
isNaNF(float x)
{
    return MathUtils::isNaN(x);
}

int
countNaN(const float* p,
         int n)
{
    // values loaded from memory, as in Image::checkForNaNsAndFix()
    int c = 0;

    for (int i = 0; i < n; ++i) {
        if ( MathUtils::isNaN(p[i]) ) {
            ++c;
        }
    }

    return c;
}

bool
isSumFinite(double a,
            double b)
{
    // a value computed in this fast-math TU
    double s = a + b;

    return MathUtils::isFinite(s);
}

} // namespace NaNGuardsFastMath

NATRON_NAMESPACE_EXIT
