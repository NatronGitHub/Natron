/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2013-2017 INRIA and Alexandre Gauthier-Foichat
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

NATRON_NAMESPACE_ENTER;

class RotoBezierTriangulation
{
public:
    RotoBezierTriangulation();

    enum VertexPointsSetEnum
    {
        // The vertex belongs to the feather polygon
        eVertexPointsSetFeather,

        // The vertex belongs to the internal shape
        eVertexPointsSetInternalShape,

        // The vertex belongs to the generated points
        eVertexPointsSetGeneratedPoints
    };

    struct VertexIndex {

        // Does this point belong to the feather polygon ?
        VertexPointsSetEnum origin;

        // The point index in the polygon
        int pointIndex;

        VertexIndex()
        : origin(eVertexPointsSetInternalShape)
        , pointIndex(-1)
        {

        }

    };

    
    typedef boost::shared_ptr<VertexIndex> VertexIndexPtr;


    struct BoundaryParametricPoint
    {
        double x,y,t;
        bool isInner;
        
    };


    struct PolygonData
    {

        
#ifndef NDEBUG
        // Used to check that all points passed to the vertex callback lie in the bbox
        RectD bezierBbox;
#endif


        // The polygon of the bezier and feather with parametric t and flag indicating the color of each point
        std::vector<BoundaryParametricPoint> bezierPolygon, featherPolygon;

        // List of generated points by the tesselation algorithms
        std::vector<Point> generatedPoint;

        // The mesh for the feather computed from bezeirPolygon and featherPolygon
        std::vector<BoundaryParametricPoint> featherMesh;

        // The vertices composing the internal shape, each single vertex once.
        // Each VertexIndex references a real vertex in either bezierPolygon, featherPolygon or generatedPoint
        std::vector<VertexIndex> internalShapeVertices;

        // The actual primitives to render. Each index is an index in internalShapeVertices
        std::vector<std::vector<unsigned int> > internalShapeTriangles, internalShapeTriangleFans, internalShapeTriangleStrips;
    
    };


    static void computeTriangles(const BezierPtr& bezier, TimeValue time, ViewIdx view, const RenderScale& scale,  double featherDistPixel_x, double featherDistPixel_y, PolygonData* outArgs);

    static Point getPointFromTriangulation(const PolygonData& inArgs, std::size_t index);
    static Point getPointFromTriangulation(const PolygonData& inArgs, const VertexIndex& index);

};

NATRON_NAMESPACE_EXIT;

#endif // ROTOBEZIERTRIANGULATION_H
