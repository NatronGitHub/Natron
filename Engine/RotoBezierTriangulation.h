/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
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

#ifndef ROTOBEZIERTRIANGULATION_H
#define ROTOBEZIERTRIANGULATION_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <vector>
#include <set>

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/scoped_ptr.hpp>
#endif

#include "Engine/Bezier.h"
#include "Engine/RectD.h"

#include "Engine/EngineFwd.h"

NATRON_NAMESPACE_ENTER

class RotoBezierTriangulation
{
public:
    RotoBezierTriangulation();


    /**
     * @brief A vertex used in the feather mesh rendering
     **/
    struct BezierVertex
    {
        // Coordinates + parametric t from the original segment
        double x,y;

        // If true, should be drawn with full opacity, otherwise with opacity = 0
        bool isInner;

    };


    struct PolygonData
    {

        // The vertices used to render the feather
        std::vector<BezierVertex> featherVertices;

        // The mesh for the feather computed from bezeirPolygon and featherPolygon
        // This vector contains indices corresponding to a GL_TRIANGLE_STRIP primitive
        // referencing the featherVertices
        std::vector<unsigned int> featherTriangles;

        // The vertices composing the internal shape, each single vertex once.
        // Each vertex is used in one of the primitives in internalShapeTriangles, internalShapeTriangleFans and internalShapeTriangleStrips
        std::vector<Point> internalShapeVertices;

        // The actual primitives to render. They correspond to GL_TRIANGLES, GL_TRIANGLE_FAN, GL_TRIANGLE_STRIP
        std::vector<std::vector<unsigned int> > internalShapeTriangles, internalShapeTriangleFans, internalShapeTriangleStrips;
    
    };

    /**
     * @brief Tesselate the given Bezier at the given view and time and scale. In output a set of vertices and render primitives can be fed directly to the renderer. 
     **/
    static void tesselate(const BezierPtr& bezier, TimeValue time, ViewIdx view, const RenderScale& scale, PolygonData* outArgs);

};

NATRON_NAMESPACE_EXIT

#endif // ROTOBEZIERTRIANGULATION_H
