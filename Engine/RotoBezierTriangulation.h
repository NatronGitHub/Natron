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

#include <vector>

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/scoped_ptr.hpp>
#endif

#include "Engine/Bezier.h"

#include "Engine/EngineFwd.h"

NATRON_NAMESPACE_ENTER;

class RotoBezierTriangulation
{
public:
    RotoBezierTriangulation();

    struct RotoFeatherVertex
    {
        double x,y;
        bool isInner;
    };

    struct RotoTriangleStrips
    {
        std::vector<unsigned  int> indices;
    };

    struct RotoTriangleFans
    {
        std::vector<unsigned int> indices;
    };

    struct RotoTriangles
    {
        std::vector<unsigned int> indices;
    };


    struct PolygonData
    {

        // Discretized beziers
        std::vector<std::vector<ParametricPoint> > featherPolygon;
        std::vector<std::vector<ParametricPoint> > bezierPolygon;

#ifndef NDEBUG
        // Used to check that all points passed to the vertex callback lie in the bbox
        RectD bezierBbox;
#endif

        // Union of all discretized bezier segments
        std::vector<ParametricPoint> bezierPolygonJoined;

        // The computed mesh for the feather
        std::vector<RotoFeatherVertex> featherMesh;

        // Stuff out of libtess
        std::vector<RotoTriangleFans> internalFans;
        std::vector<RotoTriangles> internalTriangles;
        std::vector<RotoTriangleStrips> internalStrips;

        // internal stuff for libtess callbacks
        boost::scoped_ptr<RotoTriangleFans> fanBeingEdited;
        boost::scoped_ptr<RotoTriangles> trianglesBeingEdited;
        boost::scoped_ptr<RotoTriangleStrips> stripsBeingEdited;
        
        unsigned int error;
    };

    static void computeTriangles(const BezierPtr& bezier, double time, ViewIdx view, unsigned int mipmapLevel,  double featherDist, PolygonData* outArgs);

};

NATRON_NAMESPACE_EXIT;

#endif // ROTOBEZIERTRIANGULATION_H
