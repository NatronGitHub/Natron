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

#include <cmath>
#include <boost/cstdint.hpp> // uintptr_t
#include <cstddef> // size_t

#include "libtess.h"

using boost::uintptr_t;
using std::size_t;
using std::vector;

NATRON_NAMESPACE_ENTER;


NATRON_NAMESPACE_ANONYMOUS_ENTER;


struct TessIndex {
    // Does this point belong to the feather polygon ?
    bool isFeather;

    // The index of the segment in the feather polygon
    std::size_t segmentIndex;

    // The point index in the segment
    std::size_t pointIndex;

};

typedef boost::shared_ptr<TessIndex> TessIndexPtr;

struct BoundaryParametricPoint
{
    double x,y,t;
    bool isInner;

};



struct PolygonCSGData
{
    std::vector<std::vector<BoundaryParametricPoint> >* featherPolygon;
    std::vector<std::vector<BoundaryParametricPoint> >* bezierPolygon;
    std::vector<TessIndexPtr> indices;
    unsigned int error;

    PolygonCSGData()
    : featherPolygon(0)
    , bezierPolygon(0)
    , indices()
    , error(0)
    {

    }
};

static void contour_begin_primitive_callback(unsigned int which,
                                          void *polygonData)
{

    PolygonCSGData* myData = (PolygonCSGData*)polygonData;
    assert(myData);
    assert(which == LIBTESS_GL_LINE_LOOP);

}

static void contour_end_primitive_callback(void *polygonData)
{
    PolygonCSGData* myData = (PolygonCSGData*)polygonData;
    assert(myData);

}

static void contour_vertex_callback(void* data /*per-vertex client data*/, void *polygonData)
{
    PolygonCSGData* myData = (PolygonCSGData*)polygonData;
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

static void contour_error_callback(unsigned int error,
                                void *polygonData)
{
    PolygonCSGData* myData = (PolygonCSGData*)polygonData;
    assert(myData);
    myData->error = error;
}

static void contour_intersection_combine_callback(double coords[3],
                                               void */*data*/[4] /*4 original vertices*/,
                                               double /*w*/[4] /*weights*/,
                                               void **dataOut,
                                               void *polygonData)
{
    /*PolygonCSGData* myData = (PolygonCSGData*)polygonData;
    assert(myData);
    assert(false);

    BoundaryParametricPoint v;
    v.x = coords[0];
    v.y = coords[1];
    v.t = 0;
    v.isInner = false;

    uintptr_t index = myData->bezierPolygonJoined.size();
    myData->bezierPolygonJoined.push_back(v);

    assert(index < myData->bezierPolygonJoined.size());
    *dataOut = reinterpret_cast<void*>(index);*/
}

NATRON_NAMESPACE_ANONYMOUS_EXIT;

/**
 * @brief Using libtess, union the feather polygon and the internal shape polygon so we find the actual exterior contour of the shape.
 **/
static void computeFeatherContour(const std::vector<std::vector<ParametricPoint> >& featherPolygon,
                                  const std::vector<std::vector<ParametricPoint> >& bezierPolygon,
                                  bool clockWise,
                                  double featherDistPixel_xParam,
                                  double featherDistPixel_yParam,
                                  std::vector<std::vector<BoundaryParametricPoint> >* featherPolygonOut,
                                  std::vector<std::vector<BoundaryParametricPoint> >* bezierPolygonOut)
{
    // Copy the feather and bezier polygon and introduce a isInner flag to determine if a point should be drawn with an inside color (full opacity) or outter
    // color (black)
    // Initialize the isInner flag to true since libtess will only emit vertices for the outter contour
    PolygonCSGData data;


    const double featherDist_x = featherDistPixel_xParam;
    const double featherDist_y = featherDistPixel_yParam;

    featherPolygonOut->resize(featherPolygon.size());
    bezierPolygonOut->resize(bezierPolygon.size());

    for (std::size_t i = 0; i < bezierPolygon.size(); ++i) {
        (*bezierPolygonOut)[i].resize(bezierPolygon[i].size());
        for (std::size_t j = 0; j < bezierPolygon[i].size(); ++j) {
            (*bezierPolygonOut)[i][j].x = bezierPolygon[i][j].x;
            (*bezierPolygonOut)[i][j].y = bezierPolygon[i][j].y;
            (*bezierPolygonOut)[i][j].t = bezierPolygon[i][j].t;
            (*bezierPolygonOut)[i][j].isInner = true;

            TessIndexPtr index(new TessIndex);
            index->isFeather = false;
            index->pointIndex = j;
            index->segmentIndex = i;
            data.indices.push_back(index);
        }
    }

    // The feather point must be offset along the normal by the feather distance.
    {

        for (std::size_t i = 0; i < featherPolygon.size(); ++i) {
            (*featherPolygonOut)[i].resize(featherPolygon[i].size());
            for (std::size_t j = 0; j < featherPolygon[i].size(); ++j) {

                // Get the prev and next feather point along the polygon to compute the
                // derivative
                const ParametricPoint* prev = 0;
                const ParametricPoint* next = 0;
                if (j > 0) {
                    prev = &featherPolygon[i][j - 1];
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
                        next = &featherPolygon[i + 1].front();
                    } else {
                        next = &featherPolygon.front().front();
                    }
                }

                {
                    double diffx = next->x - prev->x;
                    double diffy = next->y - prev->y;
                    double norm = std::sqrt( diffx * diffx + diffy * diffy );
                    double dx = (norm != 0) ? -( diffy / norm ) : 0;
                    double dy = (norm != 0) ? ( diffx / norm ) : 1;

                    if (!clockWise) {
                        (*featherPolygonOut)[i][j].x -= dx * featherDist_x;
                        (*featherPolygonOut)[i][j].y -= dy * featherDist_y;
                    } else {
                        (*featherPolygonOut)[i][j].x += dx * featherDist_x;
                        (*featherPolygonOut)[i][j].y += dy * featherDist_y;
                    }
                }

                (*featherPolygonOut)[i][j].t = featherPolygon[i][j].t;
                (*featherPolygonOut)[i][j].isInner = true;

                TessIndexPtr index(new TessIndex);
                index->isFeather = true;
                index->pointIndex = j;
                index->segmentIndex = i;
                data.indices.push_back(index);
            }
        }
    }

    data.featherPolygon = featherPolygonOut;
    data.bezierPolygon = bezierPolygonOut;

    libtess_GLUtesselator* tesselator = libtess_gluNewTess();

    libtess_gluTessProperty(tesselator, LIBTESS_GLU_TESS_BOUNDARY_ONLY, 1);
    libtess_gluTessProperty(tesselator, LIBTESS_GLU_TESS_WINDING_RULE, LIBTESS_GLU_TESS_WINDING_POSITIVE);
    libtess_gluTessCallback(tesselator, LIBTESS_GLU_TESS_BEGIN_DATA, (void (*)())contour_begin_primitive_callback);
    libtess_gluTessCallback(tesselator, LIBTESS_GLU_TESS_VERTEX_DATA, (void (*)())contour_vertex_callback);
    libtess_gluTessCallback(tesselator, LIBTESS_GLU_TESS_END_DATA, (void (*)())contour_end_primitive_callback);
    libtess_gluTessCallback(tesselator, LIBTESS_GLU_TESS_ERROR_DATA, (void (*)())contour_error_callback);
    libtess_gluTessCallback(tesselator, LIBTESS_GLU_TESS_COMBINE_DATA, (void (*)())contour_intersection_combine_callback);

    // From README: if you know that all polygons lie in the x-y plane, call
    // gluTessNormal(tess, 0.0, 0.0, 1.0) before rendering any polygons.
    libtess_gluTessNormal(tesselator, 0, 0, clockWise ? -1 : 1);

    // Draw a single polygon with 2 contours
    libtess_gluTessBeginPolygon(tesselator, &data);

    // The index in the list of TessIndex associated to each point
    std::size_t indexInIndicesList = 0;
    {
        libtess_gluTessBeginContour(tesselator);

        for (std::size_t i = 0; i < bezierPolygonOut->size(); ++i) {
            for (std::size_t j = 0; j < (*bezierPolygonOut)[i].size(); ++j) {
                double coords[3] = {(*bezierPolygonOut)[i][j].x, (*bezierPolygonOut)[i][j].y, 1.};
                libtess_gluTessVertex(tesselator, coords, reinterpret_cast<void*>(data.indices[indexInIndicesList].get()) /*per-vertex client data*/);
                ++indexInIndicesList;
            }
        }

        libtess_gluTessEndContour(tesselator);
    }
    {
        libtess_gluTessBeginContour(tesselator);


        for (std::size_t i = 0; i < featherPolygonOut->size(); ++i) {
            for (std::size_t j = 0; j < (*featherPolygonOut)[i].size(); ++j) {
                double coords[3] = {(*featherPolygonOut)[i][j].x, (*featherPolygonOut)[i][j].y, 1.};
                libtess_gluTessVertex(tesselator, coords, reinterpret_cast<void*>(data.indices[indexInIndicesList].get()) /*per-vertex client data*/);
                ++indexInIndicesList;
            }
        }

        libtess_gluTessEndContour(tesselator);
    }
    libtess_gluTessEndPolygon(tesselator);
    libtess_gluDeleteTess(tesselator);

    // check for errors
    assert(data.error == 0);

} // computeFeatherContour



/**
 * @brief Using the feather polygon and internal shape polygon and assuming they have the same number of points, iterate through each 
 * point to create a mesh composed of triangles.
 **/
static void computeFeatherMesh(const std::vector<std::vector<ParametricPoint> >& featherPolygonIn,
                               const std::vector<std::vector<ParametricPoint> >& bezierPolygonIn,
                               bool clockWise,
                               double featherDistPixel_xParam,
                               double featherDistPixel_yParam,
                               RotoBezierTriangulation::PolygonData* outArgs)
{

    std::vector<std::vector<BoundaryParametricPoint> > featherPolygon, bezierPolygon;
    computeFeatherContour(featherPolygonIn, bezierPolygonIn, clockWise, featherDistPixel_xParam, featherDistPixel_yParam, &featherPolygon, &bezierPolygon);



    // The discretized bezier polygon and feather polygon must have the same number of samples.
    assert( !featherPolygon.empty() && !bezierPolygon.empty() && featherPolygon.size() == bezierPolygon.size());

    // Points to the current feather bezier segment
    vector<vector<BoundaryParametricPoint> >::const_iterator fIt = featherPolygon.begin();

    // Points to the previous feather segment
    vector<vector<BoundaryParametricPoint> > ::const_iterator prevFSegmentIt = featherPolygon.end();
    --prevFSegmentIt;

    // Points to the next feather segment
    vector<vector<BoundaryParametricPoint> > ::const_iterator nextFSegmentIt = featherPolygon.begin();
    ++nextFSegmentIt;
    for (vector<vector<BoundaryParametricPoint> > ::const_iterator it = bezierPolygon.begin(); it != bezierPolygon.end(); ++it, ++fIt) {

        // Adjust prev/next segment iterators if they reach end: make them loop to begin
        if (prevFSegmentIt == featherPolygon.end()) {
            prevFSegmentIt = featherPolygon.begin();
        }
        if (nextFSegmentIt == featherPolygon.end()) {
            nextFSegmentIt = featherPolygon.begin();
        }


        // Iterate over each bezier segment.
        // There are the same number of bezier segments for the feather and the internal bezier. Each discretized segment is a contour (list of vertices)
        assert(!it->empty() && !fIt->empty());


        // Points to the current point of the bezier polygon
        vector<BoundaryParametricPoint>::const_iterator bSegmentIt = it->begin();

        // Points to the current point of the feather polygon
        vector<BoundaryParametricPoint>::const_iterator fSegmentIt = fIt->begin();


        // prepare iterators to compute derivatives for feather distance
        // fnext points to the next point in the feather polygon
        vector<BoundaryParametricPoint>::const_iterator fnext = fSegmentIt;
        ++fnext;  // can only be valid since we assert the list is not empty
        if ( fnext == fIt->end() ) {
            fnext = fIt->begin();
        }

        // fprev points to the previous feather point in the polygon. Since it is part of another segment,
        // we take the last point in the previous segment
        vector<BoundaryParametricPoint>::const_iterator fprev = prevFSegmentIt->end();
        --fprev; // can only be valid since we assert the list is not empty

        // Decrement a second time because the last point in the previous segment is equal to the first point in this segment
        assert(std::abs(fprev->x - fSegmentIt->x) < 1e-6 && std::abs(fprev->y - fSegmentIt->y) < 1e-6);
        --fprev;


        // Initialize the state with a segment between the first inner vertex and first outter vertex
        RotoBezierTriangulation::RotoFeatherVertex lastInnerVert,lastOutterVert;

        // Add an internal point
        if ( bSegmentIt != it->end() ) {
            lastInnerVert.x = bSegmentIt->x;
            lastInnerVert.y = bSegmentIt->y;
            lastInnerVert.isInner = bSegmentIt->isInner;
            outArgs->featherMesh.push_back(lastInnerVert);
            ++bSegmentIt;
        }

        // Add a feather point
        if ( fSegmentIt != fIt->end() ) {
            lastOutterVert.x = fSegmentIt->x;
            lastOutterVert.y = fSegmentIt->y;

            {
                double diffx = fnext->x - fprev->x;
                double diffy = fnext->y - fprev->y;
                double norm = std::sqrt( diffx * diffx + diffy * diffy );
                double dx = (norm != 0) ? -( diffy / norm ) : 0;
                double dy = (norm != 0) ? ( diffx / norm ) : 1;

                if (!clockWise) {
                    lastOutterVert.x -= dx * featherDist_x;
                    lastOutterVert.y -= dy * featherDist_y;
                } else {
                    lastOutterVert.x += dx * featherDist_x;
                    lastOutterVert.y += dy * featherDist_y;
                }
            }

            lastOutterVert.isInner = fSegmentIt->isInner;
            outArgs->featherMesh.push_back(lastOutterVert);
            ++fSegmentIt;
        }

        // Increment fprev/fnext for next iteration
        if ( fprev != prevFSegmentIt->end() ) {
            ++fprev;
        }
        if ( fnext != fIt->end() ) {
            ++fnext;
        }


        for (;;) {

            if ( fnext == fIt->end() ) {
                fnext = nextFSegmentIt->begin();
                ++fnext;
            }
            if ( fprev == prevFSegmentIt->end() ) {
                fprev = fIt->begin();
                ++fprev;
            }

            double inner_t = (double)INT_MAX;
            double outter_t = (double)INT_MAX;
            bool gotOne = false;
            if (bSegmentIt != it->end()) {
                inner_t = bSegmentIt->t;
                gotOne = true;
            }
            if (fSegmentIt != fIt->end()) {
                outter_t = fSegmentIt->t;
                gotOne = true;
            }

            if (!gotOne) {
                break;
            }

            // Pick the point with the minimum t
            if (inner_t <= outter_t) {
                if ( bSegmentIt != fIt->end() ) {
                    lastInnerVert.x = bSegmentIt->x;
                    lastInnerVert.y = bSegmentIt->y;
                    lastInnerVert.isInner = bSegmentIt->isInner;
                    outArgs->featherMesh.push_back(lastInnerVert);
                    ++bSegmentIt;
                }
            } else {
                if ( fSegmentIt != fIt->end() ) {
                    lastOutterVert.x = fSegmentIt->x;
                    lastOutterVert.y = fSegmentIt->y;

                    {
                        double diffx = fnext->x - fprev->x;
                        double diffy = fnext->y - fprev->y;
                        double norm = std::sqrt( diffx * diffx + diffy * diffy );
                        double dx = (norm != 0) ? -( diffy / norm ) : 0;
                        double dy = (norm != 0) ? ( diffx / norm ) : 1;

                        if (!clockWise) {
                            lastOutterVert.x -= dx * featherDist_x;
                            lastOutterVert.y -= dy * featherDist_y;
                        } else {
                            lastOutterVert.x += dx * featherDist_x;
                            lastOutterVert.y += dy * featherDist_y;
                        }
                    }
                    lastOutterVert.isInner = fSegmentIt->isInner;
                    outArgs->featherMesh.push_back(lastOutterVert);
                    ++fSegmentIt;
                }

                if ( fprev != fIt->end() ) {
                    ++fprev;
                }
                if ( fnext != fIt->end() ) {
                    ++fnext;
                }
            }

            // Initialize the first segment of the next triangle if we did not reach the end
            if (fSegmentIt == fIt->end() && bSegmentIt == it->end()) {
                break;
            }
            outArgs->featherMesh.push_back(lastOutterVert);
            outArgs->featherMesh.push_back(lastInnerVert);

            
        } // for(;;)

        // Increment iterators for next segment
        if (prevFSegmentIt != featherPolygon.end()) {
            ++prevFSegmentIt;
        }
        if (nextFSegmentIt != featherPolygon.end()) {
            ++nextFSegmentIt;
        }
    } // for each bezier segment

} // computeFeatherMesh



NATRON_NAMESPACE_ANONYMOUS_ENTER;



static void tess_begin_primitive_callback(unsigned int which,
                                          void *polygonData)
{

    RotoBezierTriangulation::PolygonData* myData = (RotoBezierTriangulation::PolygonData*)polygonData;
    assert(myData);

    switch (which) {
        case LIBTESS_GL_TRIANGLE_STRIP:
            assert(!myData->stripsBeingEdited);
            myData->stripsBeingEdited.reset(new RotoBezierTriangulation::RotoTriangleStrips);
            break;
        case LIBTESS_GL_TRIANGLE_FAN:
            assert(!myData->fanBeingEdited);
            myData->fanBeingEdited.reset(new RotoBezierTriangulation::RotoTriangleFans);
            break;
        case LIBTESS_GL_TRIANGLES:
            assert(!myData->trianglesBeingEdited);
            myData->trianglesBeingEdited.reset(new RotoBezierTriangulation::RotoTriangles);
            break;
        default:
            assert(false);
            break;
    }
}

static void tess_end_primitive_callback(void *polygonData)
{
    RotoBezierTriangulation::PolygonData* myData = (RotoBezierTriangulation::PolygonData*)polygonData;
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
    RotoBezierTriangulation::PolygonData* myData = (RotoBezierTriangulation::PolygonData*)polygonData;
    assert(myData);

    assert((myData->stripsBeingEdited && !myData->fanBeingEdited && !myData->trianglesBeingEdited) ||
           (!myData->stripsBeingEdited && myData->fanBeingEdited && !myData->trianglesBeingEdited) ||
           (!myData->stripsBeingEdited && !myData->fanBeingEdited && myData->trianglesBeingEdited));

    uintptr_t index = reinterpret_cast<uintptr_t>(data);
    assert(/**p >= 0 &&*/ index < myData->bezierPolygonJoined.size());
#ifndef NDEBUG
    assert(myData->bezierPolygonJoined[index].x >= myData->bezierBbox.x1 && myData->bezierPolygonJoined[index].x <= myData->bezierBbox.x2 &&
           myData->bezierPolygonJoined[index].y >= myData->bezierBbox.y1 && myData->bezierPolygonJoined[index].y <= myData->bezierBbox.y2);
#endif
    if (myData->stripsBeingEdited) {
        myData->stripsBeingEdited->indices.push_back(index);
    } else if (myData->fanBeingEdited) {
        myData->fanBeingEdited->indices.push_back(index);
    } else if (myData->trianglesBeingEdited) {
        myData->trianglesBeingEdited->indices.push_back(index);
    }

}

static void tess_error_callback(unsigned int error,
                                void *polygonData)
{
    RotoBezierTriangulation::PolygonData* myData = (RotoBezierTriangulation::PolygonData*)polygonData;
    assert(myData);
    myData->error = error;
}

static void tess_intersection_combine_callback(double coords[3],
                                               void */*data*/[4] /*4 original vertices*/,
                                               double /*w*/[4] /*weights*/,
                                               void **dataOut,
                                               void *polygonData)
{
    RotoBezierTriangulation::PolygonData* myData = (RotoBezierTriangulation::PolygonData*)polygonData;
    assert(myData);

    ParametricPoint v;
    v.x = coords[0];
    v.y = coords[1];
    v.t = 0;

    uintptr_t index = myData->bezierPolygonJoined.size();
    myData->bezierPolygonJoined.push_back(v);

    assert(index < myData->bezierPolygonJoined.size());
    *dataOut = reinterpret_cast<void*>(index);
}

NATRON_NAMESPACE_ANONYMOUS_EXIT;


/**
 * @brief Uses libtess to compute the primitives used to render the internal shape
 **/
static void computeInternalShapePrimitives(bool clockWise, RotoBezierTriangulation::PolygonData* outArgs)
{

    libtess_GLUtesselator* tesselator = libtess_gluNewTess();

    libtess_gluTessProperty(tesselator, LIBTESS_GLU_TESS_WINDING_RULE, LIBTESS_GLU_TESS_WINDING_POSITIVE);
    libtess_gluTessCallback(tesselator, LIBTESS_GLU_TESS_BEGIN_DATA, (void (*)())tess_begin_primitive_callback);
    libtess_gluTessCallback(tesselator, LIBTESS_GLU_TESS_VERTEX_DATA, (void (*)())tess_vertex_callback);
    libtess_gluTessCallback(tesselator, LIBTESS_GLU_TESS_END_DATA, (void (*)())tess_end_primitive_callback);
    libtess_gluTessCallback(tesselator, LIBTESS_GLU_TESS_ERROR_DATA, (void (*)())tess_error_callback);
    libtess_gluTessCallback(tesselator, LIBTESS_GLU_TESS_COMBINE_DATA, (void (*)())tess_intersection_combine_callback);

    // From README: if you know that all polygons lie in the x-y plane, call
    // gluTessNormal(tess, 0.0, 0.0, 1.0) before rendering any polygons.
    libtess_gluTessNormal(tesselator, 0, 0, clockWise ? -1 : 1);
    libtess_gluTessBeginPolygon(tesselator, outArgs);
    libtess_gluTessBeginContour(tesselator);



    for (size_t i = 0; i < outArgs->bezierPolygonJoined.size(); ++i) {
        double coords[3] = {outArgs->bezierPolygonJoined[i].x, outArgs->bezierPolygonJoined[i].y, 1.};
        uintptr_t index = i;
        assert(index < outArgs->bezierPolygonJoined.size());
        libtess_gluTessVertex(tesselator, coords, reinterpret_cast<void*>(index) /*per-vertex client data*/);
    }

    libtess_gluTessEndContour(tesselator);
    libtess_gluTessEndPolygon(tesselator);
    libtess_gluDeleteTess(tesselator);

    // check for errors
    assert(outArgs->error == 0);
} // computeInternalShapePrimitives

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
    outArgs->error = 0;

    bool clockWise = bezier->isClockwiseOriented(time, view);

    RectD featherPolyBBox;
    featherPolyBBox.setupInfinity();

    std::vector<std::vector<ParametricPoint> > featherPolygon;
    std::vector<std::vector<ParametricPoint> > bezierPolygon;


    bezier->evaluateFeatherPointsAtTime(time, view, scale, Bezier::eDeCastelJauAlgorithmIterative, -1, 1., true /*evaluateIfEqual*/, &featherPolygon, 0, &featherPolyBBox);
    bezier->evaluateAtTime(time, view, scale, Bezier::eDeCastelJauAlgorithmIterative, -1, 1., &bezierPolygon, 0,
#ifndef NDEBUG
                                       &outArgs->bezierBbox
#else
                                       0
#endif
                                       );

    // Join the internal polygon into a single vector of vertices, we will need the indices for libtess
    for (vector<vector<ParametricPoint> >::const_iterator it = bezierPolygon.begin(); it != bezierPolygon.end(); ++it) {

        // don't add the first vertex which is the same as the last vertex of the last segment
        vector<ParametricPoint>::const_iterator start = it->begin();
        ++start;
        outArgs->bezierPolygonJoined.insert(outArgs->bezierPolygonJoined.end(), start, it->end());
        
    }


    // First compute the mesh composed of triangles of the feather.
    computeFeatherMesh(featherPolygon, bezierPolygon, clockWise, featherDistPixel_x, featherDistPixel_y, outArgs);

    // Tesselate the internal bezier
    computeInternalShapePrimitives(clockWise, outArgs);



} // computeTriangles

NATRON_NAMESPACE_EXIT;
