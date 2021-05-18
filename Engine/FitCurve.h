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

#ifndef Natron_Engine_FitCurve_h
#define Natron_Engine_FitCurve_h

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <vector>

#include "Global/GlobalDefines.h"

#include "Engine/EngineFwd.h"

NATRON_NAMESPACE_ENTER

/**
 * Utility functions to fit a bezier curve to a set of points
 **/
namespace FitCurve {
struct SimpleBezierCP
{
    Point p;
    Point leftTan, rightTan;
};

/**
 * @brief Fit a Bezier curve to a (sub)set of digitized points
 * @param points Array of digitized points
 * @param error User-defined error squared
 * @param generatedBezier[out] The fitted bezier generated
 **/
void fit_cubic(const std::vector<Point>& points, double error, std::vector<SimpleBezierCP>* generatedBezier);
}

NATRON_NAMESPACE_EXIT

#endif // Natron_Engine_FitCurve_h
