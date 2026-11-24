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

#include "Global/Macros.h"

#include <vector>

#include <gtest/gtest.h>

#include "Engine/PyExprUtils.h"

NATRON_NAMESPACE_USING
NATRON_PYTHON_NAMESPACE_USING

// The expected values are the x86_64 results of the original code, i.e. the
// Natron 2.5 results on Intel. They are the same on every architecture since a
// negative double is no longer converted directly to uint32_t (undefined
// behaviour: x86_64 wraps it, arm64 clamped it to 0). #1678

TEST(ExprUtils, CellNoiseNegativeCells)
{
    const Double3DTuple origin = {0., 0., 0.};
    const Double3DTuple minusX = {-1., 0., 0.};
    const Double3DTuple minusAll = {-1., -1., -1.};
    const Double3DTuple mixed = {-2.5, 3.25, -7.75};

    EXPECT_DOUBLE_EQ(0.34227020417858617, ExprUtils::cellnoise(origin));
    EXPECT_NE(ExprUtils::cellnoise(origin), ExprUtils::cellnoise(minusX))
        << "cell -1 must not be cell 0";
    EXPECT_DOUBLE_EQ(0.96622879709262144, ExprUtils::cellnoise(minusX));
    EXPECT_DOUBLE_EQ(0.91133128686606213, ExprUtils::cellnoise(minusAll));
    EXPECT_DOUBLE_EQ(0.89680039577577275, ExprUtils::cellnoise(mixed));

    // ccellnoise() goes through the same conversion
    EXPECT_DOUBLE_EQ(0.96622879709262144, ExprUtils::ccellnoise(minusX).x);
}

TEST(ExprUtils, HashNegativeArgs)
{
    EXPECT_DOUBLE_EQ(0.40419923151009696, ExprUtils::hash(std::vector<double>(1, 1.)));
    EXPECT_DOUBLE_EQ(0.91860309287873165, ExprUtils::hash(std::vector<double>(1, -1.)));
    EXPECT_DOUBLE_EQ(0.50928603450518239, ExprUtils::hash(std::vector<double>(1, -0.3)));
    EXPECT_DOUBLE_EQ(0.60700503122224592, ExprUtils::hash(std::vector<double>(1, -42.)));
}
