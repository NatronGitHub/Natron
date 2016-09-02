/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2016 INRIA and Alexandre Gauthier-Foichat
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

#include "BezierCPSerialization.h"

SERIALIZATION_NAMESPACE_ENTER

void
BezierCPSerialization::encode(YAML_NAMESPACE::Emitter& em) const
{
    // The curves all have the same size
    assert(xCurve.keys.size() == yCurve.keys.size() && xCurve.keys.size() == leftCurveX.keys.size() && xCurve.keys.size() == leftCurveY.keys.size() && xCurve.keys.size() == rightCurveX.keys.size() && xCurve.keys.size() == rightCurveY.keys.size());

    if (xCurve.keys.empty()) {
        em << YAML_NAMESPACE::Flow << YAML_NAMESPACE::BeginSeq;
        em << x << y << leftX << leftY << rightX << rightY;
        em << YAML_NAMESPACE::EndSeq;
    } else {
        em << YAML_NAMESPACE::BeginSeq;
        xCurve.encode(em);
        yCurve.encode(em);
        leftCurveX.encode(em);
        leftCurveY.encode(em);
        rightCurveX.encode(em);
        rightCurveY.encode(em);
        em << YAML_NAMESPACE::EndSeq;
    }
}

static void decodeBezierCPCurve(const YAML_NAMESPACE::Node& node, double* x, CurveSerialization* curve)
{
    try {
        *x = node.as<double>();
    } catch (const YAML_NAMESPACE::BadConversion& /*e*/) {
        curve->decode(node);
        if (!curve->keys.empty()) {
            *x = curve->keys.front().value;
        }
    }
}

void
BezierCPSerialization::decode(const YAML_NAMESPACE::Node& node)
{
    if (!node.IsSequence() || node.size() != 6) {
        throw YAML_NAMESPACE::InvalidNode();
    }

    decodeBezierCPCurve(node[0], &x, &xCurve);
    decodeBezierCPCurve(node[1], &y, &yCurve);
    decodeBezierCPCurve(node[2], &leftX, &leftCurveX);
    decodeBezierCPCurve(node[3], &leftY, &leftCurveY);
    decodeBezierCPCurve(node[4], &rightX, &rightCurveX);
    decodeBezierCPCurve(node[5], &rightY, &rightCurveY);

}

SERIALIZATION_NAMESPACE_EXIT
