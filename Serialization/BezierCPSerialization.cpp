/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * Copyright (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
 * Copyright (C) 2018-2020 The Natron developers
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


#include "BezierCPSerialization.h"

GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_OFF
#include <yaml-cpp/yaml.h>
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_ON

SERIALIZATION_NAMESPACE_ENTER

void
BezierCPSerialization::encode(YAML::Emitter& em) const
{
    // The curves all have the same size
    assert(xCurve.keys.size() == yCurve.keys.size() && xCurve.keys.size() == leftCurveX.keys.size() && xCurve.keys.size() == leftCurveY.keys.size() && xCurve.keys.size() == rightCurveX.keys.size() && xCurve.keys.size() == rightCurveY.keys.size());

    if (xCurve.keys.empty()) {
        em << YAML::Flow << YAML::BeginSeq;
        em << x << y << leftX << leftY << rightX << rightY;
        em << YAML::EndSeq;
    } else {
        em << YAML::Flow << YAML::BeginSeq;
        xCurve.encode(em);
        yCurve.encode(em);
        leftCurveX.encode(em);
        leftCurveY.encode(em);
        rightCurveX.encode(em);
        rightCurveY.encode(em);
        em << YAML::EndSeq;
    }
}

static void decodeBezierCPCurve(const YAML::Node& node, double* x, CurveSerialization* curve)
{
    try {
        if (node.IsSequence()) {
            curve->decode(node);
            if (!curve->keys.empty()) {
                *x = curve->keys.front().value;
            }
        } else {
            *x = node.as<double>();
        }
    } catch (const YAML::BadConversion& /*e*/) {
        assert(false);
    }
}

void
BezierCPSerialization::decode(const YAML::Node& node)
{
    if (!node.IsSequence() || node.size() != 6) {
        throw YAML::InvalidNode();
    }

    decodeBezierCPCurve(node[0], &x, &xCurve);
    decodeBezierCPCurve(node[1], &y, &yCurve);
    decodeBezierCPCurve(node[2], &leftX, &leftCurveX);
    decodeBezierCPCurve(node[3], &leftY, &leftCurveY);
    decodeBezierCPCurve(node[4], &rightX, &rightCurveX);
    decodeBezierCPCurve(node[5], &rightY, &rightCurveY);

}

SERIALIZATION_NAMESPACE_EXIT
