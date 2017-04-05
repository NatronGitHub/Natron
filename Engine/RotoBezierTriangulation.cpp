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

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "RotoBezierTriangulation.h"

#include <QDebug>
#include <cmath>
#include <set>
#include <boost/cstdint.hpp> // uintptr_t
#include <cstddef> // size_t

#include "libtess.h"

using boost::uintptr_t;
using std::size_t;
using std::vector;


NATRON_NAMESPACE_ENTER;


NATRON_NAMESPACE_ANONYMOUS_ENTER;




struct VertexIndexCompare {
    bool operator() (const RotoBezierTriangulation::VertexIndex& lhs, const RotoBezierTriangulation::VertexIndex& rhs) const
    {
        if (lhs.origin < rhs.origin) {
            return true;
        } else if (lhs.origin > rhs.origin) {
            return false;
        }

        return lhs.pointIndex < rhs.pointIndex;
    }
};

typedef std::set<RotoBezierTriangulation::VertexIndex, VertexIndexCompare> VertexIndexSet;
typedef std::vector<RotoBezierTriangulation::VertexIndex> VertexIndexVector;


struct PolygonCSGData
{

    std::vector<RotoBezierTriangulation::VertexIndexPtr> indices;
    RotoBezierTriangulation::PolygonData* outArgs;
    
    // The polygon of the bezier and feather with parametric t and flag indicating the color of each point
    std::vector<RotoBezierTriangulation::BoundaryParametricPoint> bezierPolygon, featherPolygon;
    
    // List of generated points by the tesselation algorithms
    std::vector<Point> generatedPoint;

    // The primitives to render the internal shape. Each VertexIndex refers to a point
    // in bezierPolygon or featherPolygon
    std::vector<VertexIndexVector> internalFans, internalTriangles, internalStrips;

    // Uniquely record each vertex composing the internal shape
    VertexIndexSet internalShapeVertices;

    // internal stuff for libtess callbacks
    boost::scoped_ptr<VertexIndexVector> fanBeingEdited;
    boost::scoped_ptr<VertexIndexVector> trianglesBeingEdited;
    boost::scoped_ptr<VertexIndexVector> stripsBeingEdited;

    unsigned int error;

    PolygonCSGData()
    : indices()
    , outArgs(0)
    , fanBeingEdited()
    , trianglesBeingEdited()
    , stripsBeingEdited()
    , error(0)
    {

    }
};

static void tess_begin_primitive_callback(unsigned int which,
                                          void *polygonData)
{

    PolygonCSGData* myData = (PolygonCSGData*)polygonData;
    assert(myData);

    switch (which) {
        case LIBTESS_GL_TRIANGLE_STRIP:
            assert(!myData->stripsBeingEdited);
            myData->stripsBeingEdited.reset(new VertexIndexVector);
            break;
        case LIBTESS_GL_TRIANGLE_FAN:
            assert(!myData->fanBeingEdited);
            myData->fanBeingEdited.reset(new VertexIndexVector);
            break;
        case LIBTESS_GL_TRIANGLES:
            assert(!myData->trianglesBeingEdited);
            myData->trianglesBeingEdited.reset(new VertexIndexVector);
            break;
        default:
            assert(false);
            break;
    }
}

static void tess_end_primitive_callback(void *polygonData)
{
    PolygonCSGData* myData = (PolygonCSGData*)polygonData;
    assert(myData);

    assert((myData->stripsBeingEdited && !myData->fanBeingEdited && !myData->trianglesBeingEdited) ||
           (!myData->stripsBeingEdited && myData->fanBeingEdited && !myData->trianglesBeingEdited) ||
           (!myData->stripsBeingEdited && !myData->fanBeingEdited && myData->trianglesBeingEdited));

    if (myData->stripsBeingEdited) {
        myData->internalStrips.push_back(*myData->stripsBeingEdited);
        myData->stripsBeingEdited.reset();
    } else if (myData->fanBeingEdited) {
        myData->internalFans.push_back(*myData->fanBeingEdited);
        myData->fanBeingEdited.reset();
    } else if (myData->trianglesBeingEdited) {
        myData->internalTriangles.push_back(*myData->trianglesBeingEdited);
        myData->trianglesBeingEdited.reset();
    }
}

static void tess_vertex_callback(void* data /*per-vertex client data*/,
                                 void *polygonData)
{
    PolygonCSGData* myData = (PolygonCSGData*)polygonData;
    assert(myData);

    assert((myData->stripsBeingEdited && !myData->fanBeingEdited && !myData->trianglesBeingEdited) ||
           (!myData->stripsBeingEdited && myData->fanBeingEdited && !myData->trianglesBeingEdited) ||
           (!myData->stripsBeingEdited && !myData->fanBeingEdited && myData->trianglesBeingEdited));


    RotoBezierTriangulation::VertexIndex* index = (RotoBezierTriangulation::VertexIndex*)data;

    myData->internalShapeVertices.insert(*index);

    // If the vertex was originally part of our feather polygon or internal polygon, mark that it is assumed to be in the
    // intersection of both polygons.
    switch (index->origin) {
        case RotoBezierTriangulation::eVertexPointsSetFeather:
            assert(index->pointIndex < (int)myData->featherPolygon.size());
            myData->featherPolygon[index->pointIndex].isInner = true;
            break;
        case RotoBezierTriangulation::eVertexPointsSetInternalShape:
            assert(index->pointIndex < (int)myData->bezierPolygon.size());
            myData->bezierPolygon[index->pointIndex].isInner = true;

            break;
        case RotoBezierTriangulation::eVertexPointsSetGeneratedPoints:
            break;
    }


    if (myData->stripsBeingEdited) {
        myData->stripsBeingEdited->push_back(*index);
    } else if (myData->fanBeingEdited) {
        myData->fanBeingEdited->push_back(*index);
    } else if (myData->trianglesBeingEdited) {
        myData->trianglesBeingEdited->push_back(*index);
    }

}

static void tess_error_callback(unsigned int error,
                                void *polygonData)
{
    PolygonCSGData* myData = (PolygonCSGData*)polygonData;
    assert(myData);
    myData->error = error;
}

static void tess_intersection_combine_callback(double coords[3],
                                               void */*data*/[4] /*4 original vertices*/,
                                               double /*w*/[4] /*weights*/,
                                               void **dataOut,
                                               void *polygonData)
{
    // We are at an intersection of the feather and internal bezier.
    // We add a point to the bezier polygon.
    // We are not going to use it anyway afterwards, 
    PolygonCSGData* myData = (PolygonCSGData*)polygonData;
    assert(myData);

    Point v;
    v.x = coords[0];
    v.y = coords[1];
    myData->generatedPoint.push_back(v);

    RotoBezierTriangulation::VertexIndexPtr index(new RotoBezierTriangulation::VertexIndex);
    index->origin = RotoBezierTriangulation::eVertexPointsSetGeneratedPoints;
    index->pointIndex = myData->generatedPoint.size() - 1;
    *dataOut = (void*)index.get();
    myData->indices.push_back(index);
}


static Point
getPointFromTriangulation(const PolygonCSGData& inArgs, const RotoBezierTriangulation::VertexIndex& index)
{
    Point p;
    switch (index.origin) {
        case RotoBezierTriangulation::eVertexPointsSetFeather:
            assert(index.pointIndex < (int)inArgs.featherPolygon.size());
            p.x = inArgs.featherPolygon[index.pointIndex].x;
            p.y = inArgs.featherPolygon[index.pointIndex].y;
            break;
            
        case RotoBezierTriangulation::eVertexPointsSetInternalShape:
            assert(index.pointIndex < (int)inArgs.bezierPolygon.size());
            p.x = inArgs.bezierPolygon[index.pointIndex].x;
            p.y = inArgs.bezierPolygon[index.pointIndex].y;
            break;
            
        case RotoBezierTriangulation::eVertexPointsSetGeneratedPoints:
            assert(index.pointIndex < (int)inArgs.generatedPoint.size());
            p.x = inArgs.generatedPoint[index.pointIndex].x;
            p.y = inArgs.generatedPoint[index.pointIndex].y;
            break;
            
    }
    return p;
}


#if 0
struct ContourCSGData
{
    RotoBezierTriangulation::PolygonData* outArgs;
};

static void contour_begin_primitive_callback(unsigned int which,
                                             void */*polygonData*/)
{
    (void)which;
    assert(which == LIBTESS_GL_LINE_LOOP);
    // Does nothing, we just want to mark the points in contour_vertex_callback
}

static void contour_end_primitive_callback(void * /*polygonData*/)
{
    // Does nothing we just want to mark the points in contour_vertex_callback
}

static void contour_vertex_callback(void* data /*per-vertex client data*/, void *polygonData)
{
    ContourCSGData* myData = (ContourCSGData*)polygonData;
    assert(myData);
    TessIndex* index = (TessIndex*)data;

    if (index->isFeather) {
        assert(index->segmentIndex < myData->featherPolygon->size());
        std::vector<BoundaryParametricPoint>& segment = (*myData->featherPolygon)[index->segmentIndex];
        assert(index->pointIndex < segment.size());
        segment[index->pointIndex].isInner = false;

    } else {
        assert(index->segmentIndex < myData->bezierPolygon->size());
        std::vector<BoundaryParametricPoint>& segment = (*myData->bezierPolygon)[index->segmentIndex];
        assert(index->pointIndex < segment.size());
        segment[index->pointIndex].isInner = false;
    }

}

static void contour_error_callback(unsigned int /*error*/,
                                   void */*polygonData*/)
{
    assert(false);
}

static void contour_intersection_combine_callback(double /*coords*/[3],
                                                  void */*data*/[4] /*4 original vertices*/,
                                                  double /*w*/[4] /*weights*/,
                                                  void **dataOut,
                                                  void *polygonData)
{
    ContourCSGData* myData = (ContourCSGData*)polygonData;
    assert(myData);

    // In output return a fake index otherwise libtess will return an error
    TessIndexPtr index(new TessIndex);
    *dataOut = (void*)index.get();
    myData->indices.push_back(index);
}
#endif

NATRON_NAMESPACE_ANONYMOUS_EXIT;

static void initializePolygons(PolygonCSGData& data,
                               bool clockWise,
                               const std::vector<std::vector<ParametricPoint> >& featherPolygon,
                               const std::vector<std::vector<ParametricPoint> >& bezierPolygon,
                               double featherDistPixel_xParam,
                               double featherDistPixel_yParam)
{
    // Copy the feather and bezier polygon and introduce a isInner flag to determine if a point should be drawn with an inside color (full opacity) or outter
    // color (black)
    // Initialize the isInner flag to false and libtess will inform us of the internal points with the vertex callback
    
    
    const double featherDist_x = featherDistPixel_xParam;
    const double featherDist_y = featherDistPixel_yParam;
    
    for (std::size_t i = 0; i < bezierPolygon.size(); ++i) {
        
        // Skip the first point in input because it is the same as the last point of the previous segment
        for (std::size_t j = 1; j < bezierPolygon[i].size(); ++j) {
            const ParametricPoint& from = bezierPolygon[i][j];
            RotoBezierTriangulation::BoundaryParametricPoint to;
            to.x = from.x;
            to.y = from.y;
            // Add the index of the segment to the parametric t so further in computeFeatherMesh we do not compare 2 points
            // which belong to 2 different segments
            to.t = from.t + i;
            to.isInner = false;
            data.bezierPolygon.push_back(to);
            
            RotoBezierTriangulation::VertexIndexPtr index(new RotoBezierTriangulation::VertexIndex);
            index->origin = RotoBezierTriangulation::eVertexPointsSetInternalShape;
            index->pointIndex = data.bezierPolygon.size() - 1;
            data.indices.push_back(index);
        }
    }
    
    // The feather point must be offset along the normal by the feather distance.
    for (std::size_t i = 0; i < featherPolygon.size(); ++i) {
        
        // Skip the first point in input because it is the same as the last point of the previous segment
        for (std::size_t j = 1; j < featherPolygon[i].size(); ++j) {
            
            // Get the prev and next feather point along the polygon to compute the
            // derivative
            const ParametricPoint* prev = 0;
            const ParametricPoint* next = 0;
            
            if (j > 1) {
                prev = &featherPolygon[i][j];
            } else {
                if (i > 0) {
                    prev = &featherPolygon[i - 1].back();
                } else {
                    prev = &featherPolygon.back().back();
                }
            }
            
            if (j < featherPolygon[i].size() - 1) {
                next = &featherPolygon[i][j + 1];
            } else {
                if (i < featherPolygon.size() - 1) {
                    next = &featherPolygon[i + 1][1];
                } else {
                    next = &featherPolygon[0][1];
                }
            }
            
            const ParametricPoint& from = featherPolygon[i][j];
            RotoBezierTriangulation::BoundaryParametricPoint to;
            to.x = from.x;
            to.y = from.y;
            
            // Add the index of the segment to the parametric t so further in computeFeatherMesh we do not compare 2 points
            // which belong to 2 different segments
            to.t = from.t + i;
            to.isInner = false;
            
            // Add or remove the derivative multiplied by the feather distance, depending on the polygon orientation
            {
                double diffx = next->x - prev->x;
                double diffy = next->y - prev->y;
                double norm = std::sqrt( diffx * diffx + diffy * diffy );
                double dx = (norm != 0) ? -( diffy / norm ) : 0;
                double dy = (norm != 0) ? ( diffx / norm ) : 1;
                
                if (!clockWise) {
                    to.x -= dx * featherDist_x;
                    to.y -= dy * featherDist_y;
                } else {
                    to.x += dx * featherDist_x;
                    to.y += dy * featherDist_y;
                }
            }
            data.featherPolygon.push_back(to);
            
            
            RotoBezierTriangulation::VertexIndexPtr index(new RotoBezierTriangulation::VertexIndex);
            index->origin = RotoBezierTriangulation::eVertexPointsSetFeather;
            index->pointIndex = data.featherPolygon.size() - 1;
            data.indices.push_back(index);
        } // for each point in a segment
    } // for each feather bezier segment
} // initializePolygons

/**
 * @brief Using libtess, computes the intersection of the feather polygon and the internal shape polygon so we find the actual interior and exterior of the shape
 **/
static void computeInternalPolygon(PolygonCSGData& data, bool clockWise, RotoBezierTriangulation::PolygonData* outArgs)
{
    


    libtess_GLUtesselator* tesselator = libtess_gluNewTess();

    //libtess_gluTessProperty(tesselator, LIBTESS_GLU_TESS_BOUNDARY_ONLY, 1);
    libtess_gluTessProperty(tesselator, LIBTESS_GLU_TESS_WINDING_RULE, LIBTESS_GLU_TESS_WINDING_ABS_GEQ_TWO);
    libtess_gluTessCallback(tesselator, LIBTESS_GLU_TESS_BEGIN_DATA, (void (*)())tess_begin_primitive_callback);
    libtess_gluTessCallback(tesselator, LIBTESS_GLU_TESS_VERTEX_DATA, (void (*)())tess_vertex_callback);
    libtess_gluTessCallback(tesselator, LIBTESS_GLU_TESS_END_DATA, (void (*)())tess_end_primitive_callback);
    libtess_gluTessCallback(tesselator, LIBTESS_GLU_TESS_ERROR_DATA, (void (*)())tess_error_callback);
    libtess_gluTessCallback(tesselator, LIBTESS_GLU_TESS_COMBINE_DATA, (void (*)())tess_intersection_combine_callback);

    // From README: if you know that all polygons lie in the x-y plane, call
    // gluTessNormal(tess, 0.0, 0.0, 1.0) before rendering any polygons.
    libtess_gluTessNormal(tesselator, 0, 0, clockWise ? -1 : 1);

    // Draw a single polygon with 2 contours
    libtess_gluTessBeginPolygon(tesselator, &data);

    // The index in the list of VertexIndex associated to each point
    std::size_t indexInIndicesList = 0;
    {
        libtess_gluTessBeginContour(tesselator);

        for (std::size_t i = 0; i < data.bezierPolygon.size(); ++i) {
            double coords[3] = {data.bezierPolygon[i].x, data.bezierPolygon[i].y, 1.};
            libtess_gluTessVertex(tesselator, coords, (void*)data.indices[indexInIndicesList].get() /*per-vertex client data*/);
            ++indexInIndicesList;
        }

        libtess_gluTessEndContour(tesselator);
    }
    {
        libtess_gluTessBeginContour(tesselator);


        for (std::size_t i = 0; i < data.featherPolygon.size(); ++i) {
            double coords[3] = {data.featherPolygon[i].x, data.featherPolygon[i].y, 1.};
            libtess_gluTessVertex(tesselator, coords, (void*)data.indices[indexInIndicesList].get() /*per-vertex client data*/);
            ++indexInIndicesList;
        }

        libtess_gluTessEndContour(tesselator);
    }
    libtess_gluTessEndPolygon(tesselator);
    libtess_gluDeleteTess(tesselator);
    
    // check for errors
    assert(data.error == 0);

    // Now transfer the data to the outArgs
    {
        outArgs->internalShapeVertices.resize(data.internalShapeVertices.size());
        int i = 0;
        for (VertexIndexSet::const_iterator it = data.internalShapeVertices.begin(); it != data.internalShapeVertices.end(); ++it, ++i) {
            outArgs->internalShapeVertices[i] =  getPointFromTriangulation(data, *it);
        }
    }
    outArgs->internalShapeTriangleFans.resize(data.internalFans.size());
    outArgs->internalShapeTriangles.resize(data.internalTriangles.size());
    outArgs->internalShapeTriangleStrips.resize(data.internalStrips.size());

    // Copy triangles
    for (std::size_t i = 0; i < data.internalTriangles.size(); ++i) {
        outArgs->internalShapeTriangles[i].resize(data.internalTriangles[i].size());
        for (std::size_t j = 0; j < data.internalTriangles[i].size(); ++j) {
            const RotoBezierTriangulation::VertexIndex& index = data.internalTriangles[i][j];

            // Find the vertex in the unique set to retrieve its index
            VertexIndexSet::iterator found = data.internalShapeVertices.find(index);
            assert(found != data.internalShapeVertices.end());
            if (found != data.internalShapeVertices.end()) {
                std::size_t vertexIndexInVec = std::distance(data.internalShapeVertices.begin(), found);
                outArgs->internalShapeTriangles[i][j] = vertexIndexInVec;
            }

        }
    }

    // Copy triangle fans
    for (std::size_t i = 0; i < data.internalFans.size(); ++i) {
        outArgs->internalShapeTriangleFans[i].resize(data.internalFans[i].size());
        for (std::size_t j = 0; j < data.internalFans[i].size(); ++j) {
            const RotoBezierTriangulation::VertexIndex& index = data.internalFans[i][j];

            // Find the vertex in the unique set to retrieve its index
            VertexIndexSet::iterator found = data.internalShapeVertices.find(index);
            assert(found != data.internalShapeVertices.end());
            if (found != data.internalShapeVertices.end()) {
                std::size_t vertexIndexInVec = std::distance(data.internalShapeVertices.begin(), found);
                outArgs->internalShapeTriangleFans[i][j] = vertexIndexInVec;
            }

        }
    }

    // Copy triangle strips
    for (std::size_t i = 0; i < data.internalStrips.size(); ++i) {
        outArgs->internalShapeTriangleStrips[i].resize(data.internalStrips[i].size());
        for (std::size_t j = 0; j < data.internalStrips[i].size(); ++j) {
            const RotoBezierTriangulation::VertexIndex& index = data.internalStrips[i][j];

            // Find the vertex in the unique set to retrieve its index
            VertexIndexSet::iterator found = data.internalShapeVertices.find(index);
            assert(found != data.internalShapeVertices.end());
            if (found != data.internalShapeVertices.end()) {
                std::size_t vertexIndexInVec = std::distance(data.internalShapeVertices.begin(), found);
                outArgs->internalShapeTriangleStrips[i][j] = vertexIndexInVec;
            }

        }
    }

} // computeFeatherContour



/**
 * @brief Using the feather polygon and internal shape polygon and assuming they have the same number of points, iterate through each 
 * point to create a mesh composed of triangles.
 **/
static void computeFeatherMesh(PolygonCSGData &data, RotoBezierTriangulation::PolygonData* outArgs)
{

    // The discretized bezier polygon and feather polygon must have the same number of samples.
    assert( !data.featherPolygon.empty() && !data.bezierPolygon.empty());

    // Points to the current feather bezier segment
    vector<RotoBezierTriangulation::BoundaryParametricPoint>::const_iterator fit = data.featherPolygon.begin();
    vector<RotoBezierTriangulation::BoundaryParametricPoint> ::const_iterator it = data.bezierPolygon.begin();

    // Initialize the state with a segment between the first inner vertex and first outter vertex
    RotoBezierTriangulation::BoundaryParametricPoint lastInnerVert,lastOutterVert;
    lastInnerVert = *it;
    lastOutterVert = *fit;

    // Whether or not to increment the iterators at the start of the loop
    bool incrFeatherIt = true;
    bool incrBezierIt = true;

    while (fit != data.featherPolygon.end() || it != data.bezierPolygon.end()) {

        // Initialize the first segment of the next triangle if we did not reach the end

        outArgs->featherMesh.push_back(lastOutterVert);
        outArgs->featherMesh.push_back(lastInnerVert);
        if ( incrBezierIt && it != data.bezierPolygon.end() ) {
            ++it;
        }
        if ( incrFeatherIt && fit != data.featherPolygon.end() ) {
            ++fit;

        }
        double inner_t = (double)INT_MAX;
        double outter_t = (double)INT_MAX;
        if (it != data.bezierPolygon.end()) {
            inner_t = it->t;
        }
        if (fit != data.featherPolygon.end()) {
            outter_t = fit->t;
        }

        // Pick the point with the minimum t
        if (inner_t <= outter_t) {
            incrBezierIt = true;
            incrFeatherIt = false;
            if ( it != data.bezierPolygon.end() ) {
                lastInnerVert = *it;
                outArgs->featherMesh.push_back(lastInnerVert);
            }
        } else {
            incrBezierIt = false;
            incrFeatherIt = true;
            if ( fit != data.featherPolygon.end() ) {
                lastOutterVert = *fit;
                outArgs->featherMesh.push_back(lastOutterVert);
            }
        }
    } // for each point

} // computeFeatherMesh


void
RotoBezierTriangulation::computeTriangles(const BezierPtr& bezier,
                                          TimeValue time,
                                          ViewIdx view,
                                          const RenderScale& scale,
                                          double featherDistPixel_x,
                                          double featherDistPixel_y,
                                          PolygonData* outArgs)
{
    ///Note that we do not use the opacity when rendering the bezier, it is rendered with correct floating point opacity/color when converting
    ///to the Natron image.

    assert(outArgs);

    bool clockWise = bezier->isClockwiseOriented(time, view);

    std::vector<std::vector<ParametricPoint> > featherPolygonOrig;
    std::vector<std::vector<ParametricPoint> > bezierPolygonOrig;

    bezier->evaluateFeatherPointsAtTime(time, view, scale, Bezier::eDeCastelJauAlgorithmIterative, -1, 1., &featherPolygonOrig, 0,
#ifndef NDEBUG
                                        &outArgs->bezierBbox
#else
                                        0
#endif
                                        );
    bezier->evaluateAtTime(time, view, scale, Bezier::eDeCastelJauAlgorithmIterative, -1, 1., &bezierPolygonOrig, 0, 0);

    PolygonCSGData data;
    data.outArgs = outArgs;

    initializePolygons(data, clockWise, featherPolygonOrig, bezierPolygonOrig, featherDistPixel_x, featherDistPixel_y);
    
    // First compute the intersection of both polygons to extract the inner shape
    computeInternalPolygon(data, clockWise, outArgs);

    // Now that we have the role (inner or outter) for each vertex, compute the feather mesh
    computeFeatherMesh(data, outArgs);

} // computeTriangles



NATRON_NAMESPACE_EXIT;
