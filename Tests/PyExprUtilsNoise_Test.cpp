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

// Regression tests for ExprUtils::cnoise() (#1683) and ExprUtils::ccellnoise() (#1684),
// which must match SeExpr's cnoise and ccellnoise. The point has non-negative coordinates:
// CellNoise of a negative coordinate is the subject of #1678.

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <gtest/gtest.h>

#include "Engine/Noise.h"
#include "Engine/PyExprUtils.h"

NATRON_NAMESPACE_USING
NATRON_PYTHON_NAMESPACE_USING

// SeExpr: cnoise(p) = 0.5 * vnoise(p) + 0.5, and vnoise is Noise<3, 3>.
TEST(ExprUtilsNoise, CNoiseIsRemappedVectorNoise)
{
    const Double3DTuple p = {0.3, 1.7, 2.45};
    const Double3DTuple c = ExprUtils::cnoise(p);
    const Double3DTuple v = ExprUtils::vnoise(p);

    // cnoise used to return p * 0.5 + 0.5
    EXPECT_NE(p.x * 0.5 + 0.5, c.x);
    EXPECT_NE(p.y * 0.5 + 0.5, c.y);
    EXPECT_NE(p.z * 0.5 + 0.5, c.z);

    EXPECT_DOUBLE_EQ(0.5 * v.x + 0.5, c.x);
    EXPECT_DOUBLE_EQ(0.5 * v.y + 0.5, c.y);
    EXPECT_DOUBLE_EQ(0.5 * v.z + 0.5, c.z);
}

// SeExpr: ccellnoise(p) is CellNoise<3, 3>; its x is cellnoise(p).
TEST(ExprUtilsNoise, CCellNoiseHasThreeComponents)
{
    const Double3DTuple p = {0.3, 1.7, 2.45};
    const Double3DTuple c = ExprUtils::ccellnoise(p);

    const double in[3] = {p.x, p.y, p.z};
    double expected[3] = {0., 0., 0.};
    CellNoise<3, 3>(in, expected);

    // y and z used to be always 0
    EXPECT_NE(0., c.y);
    EXPECT_NE(0., c.z);

    EXPECT_DOUBLE_EQ(expected[0], c.x);
    EXPECT_DOUBLE_EQ(expected[1], c.y);
    EXPECT_DOUBLE_EQ(expected[2], c.z);
    EXPECT_DOUBLE_EQ(ExprUtils::cellnoise(p), c.x);
}
