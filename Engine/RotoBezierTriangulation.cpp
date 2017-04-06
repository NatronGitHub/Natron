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

    int contourIndex;

    // The point index in the polygon
    int pointIndex;

    VertexIndex()
    : origin(eVertexPointsSetInternalShape)
    , contourIndex(0)
    , pointIndex(-1)
    {

    }

};


typedef boost::shared_ptr<VertexIndex> VertexIndexPtr;




struct VertexIndexCompare {
    bool operator() (const VertexIndex& lhs, const VertexIndex& rhs) const
    {
        if (lhs.origin < rhs.origin) {
            return true;
        } else if (lhs.origin > rhs.origin) {
            return false;
        }
        if (lhs.contourIndex < rhs.contourIndex) {
            return true;
        } else if (lhs.contourIndex > rhs.contourIndex) {
            return false;
        }

        return lhs.pointIndex < rhs.pointIndex;
    }
};

typedef std::set<VertexIndex, VertexIndexCompare> VertexIndexSet;
typedef std::vector<VertexIndex> VertexIndexVector;

struct PolygonVertex
{

    // The index in the polygon
    VertexIndexPtr index;

    double x,y,t;
    bool isInner;

    // Was this an intersection created by the combined callback ?
    // If true, the t2 member is set to the parametric t of the point created
    // for the second edge.
    // Similarly, generatedOrigin2 is the vertex origin if it is an intersection
    bool isIntersectionPoint;
    double t2;

    // To which polygon this point must be inserted
    VertexPointsSetEnum generatedOrigin1;

    // To which polygon this point must be inserted
    VertexPointsSetEnum generatedOrigin2;

    // The index of the contour of the second polygon
    int contourIndex2;

};

struct PolygonCSGData
{

    RotoBezierTriangulation::PolygonData* outArgs;
    
    // The polygon of the bezier and feather with parametric t and flag indicating the color of each point
    std::vector<PolygonVertex> bezierPolygon, featherPolygon;

    // The bezierPolygon and featherPolygon contour(s) in output of ensurePolygonWindingNumberEqualsOne
    std::vector<std::vector<PolygonVertex> > bezierProcessedContours, featherProcessContours;
    
    // List of generated points by the tesselation algorithms
    std::vector<PolygonVertex> generatedPoints;

    // The primitives to render the internal shape. Each VertexIndex refers to a point
    // in bezierPolygon or featherPolygon
    std::vector<VertexIndexVector> internalFans, internalTriangles, internalStrips;

    // Uniquely record each vertex composing the internal shape
    VertexIndexSet internalShapeVertices;

    // internal stuff for libtess callbacks
    boost::scoped_ptr<VertexIndexVector> fanBeingEdited;
    boost::scoped_ptr<VertexIndexVector> trianglesBeingEdited;
    boost::scoped_ptr<VertexIndexVector> stripsBeingEdited;

#ifndef NDEBUG
    // Used to check that all points passed to the vertex callback lie in the bbox
    RectD bezierBbox;
#endif

    unsigned int error;

    PolygonCSGData()
    : outArgs(0)
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


    VertexIndex* index = (VertexIndex*)data;

    myData->internalShapeVertices.insert(*index);

    // If the vertex was originally part of our feather polygon or internal polygon, mark that it is assumed to be in the
    // intersection of both polygons.
    switch (index->origin) {
        case eVertexPointsSetFeather:
            assert(index->contourIndex < (int)myData->featherProcessContours.size());
            assert(index->pointIndex < (int)myData->featherProcessContours[index->contourIndex].size());
            myData->featherProcessContours[index->contourIndex][index->pointIndex].isInner = true;
            break;
        case eVertexPointsSetInternalShape:
            assert(index->contourIndex < (int)myData->bezierProcessedContours.size());
            assert(index->pointIndex < (int)myData->bezierProcessedContours[index->contourIndex].size());
            myData->bezierProcessedContours[index->contourIndex][index->pointIndex].isInner = true;
            break;
        case eVertexPointsSetGeneratedPoints:
            assert(index->pointIndex < (int)myData->generatedPoints.size());
            myData->generatedPoints[index->pointIndex].isInner = true;
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

static double averageParametric_t(int startIdx, PolygonCSGData* myData, VertexIndex* originalVertices[4], float weights[4])
{
    double t = 0.;
    double weightsSum = 0.;
    for (int i = startIdx; i < startIdx + 2; ++i) {
        if (originalVertices[i]) {

            weightsSum += weights[i];

            double vertexT;
            switch (originalVertices[i]->origin) {
                case eVertexPointsSetInternalShape:
                    assert(originalVertices[i]->contourIndex < (int)myData->bezierProcessedContours.size());
                    assert(originalVertices[i]->pointIndex < (int)myData->bezierProcessedContours[originalVertices[i]->contourIndex].size());
                    vertexT = myData->bezierProcessedContours[originalVertices[i]->contourIndex][originalVertices[i]->pointIndex].t;
                    break;
                case eVertexPointsSetFeather:
                    assert(originalVertices[i]->contourIndex < (int)myData->featherProcessContours.size());
                    assert(originalVertices[i]->pointIndex < (int)myData->featherProcessContours[originalVertices[i]->contourIndex].size());
                    vertexT = myData->featherProcessContours[originalVertices[i]->contourIndex][originalVertices[i]->pointIndex].t;

                    break;
                case eVertexPointsSetGeneratedPoints:
                    assert(originalVertices[i]->pointIndex < (int)myData->generatedPoints.size());
                    vertexT = myData->generatedPoints[originalVertices[i]->pointIndex].t;
                    break;
            }
            t += (weights[i] * vertexT);
        }
    }
    if (weightsSum > 0) {
        t /= weightsSum;
    }
    return t;
}

static VertexPointsSetEnum getVertexOriginFromParentOrigin(const VertexIndex& parentIndex, const PolygonCSGData& data)
{
    if (parentIndex.origin != eVertexPointsSetGeneratedPoints) {
        return parentIndex.origin;
    }

    assert(parentIndex.pointIndex < (int)data.generatedPoints.size());
    return data.generatedPoints[parentIndex.pointIndex].generatedOrigin1;
}

static void tess_intersection_combine_callback(double coords[3],
                                               void *data[4] /*4 original vertices*/,
                                               float weights[4] /*weights*/,
                                               void **dataOut,
                                               void *polygonData)
{
    // We are at an intersection of the feather and internal bezier.
    // We add a point to the bezier polygon.
    // We are not going to use it anyway afterwards, 
    PolygonCSGData* myData = (PolygonCSGData*)polygonData;
    assert(myData);

    PolygonVertex v;
    v.x = coords[0];
    v.y = coords[1];
    v.isInner = false;
    v.index.reset(new VertexIndex);
    v.index->origin = eVertexPointsSetGeneratedPoints;
    v.index->pointIndex = myData->generatedPoints.size();

    *dataOut = (void*)v.index.get();

    // To find out the parametric t of the generated point, we use the weights on the original vertices
    VertexIndex* originalVertices[4] = {(VertexIndex*)data[0], (VertexIndex*)data[1], (VertexIndex*)data[2], (VertexIndex*)data[3]};



    // There are 2 cases for which this function is called,
    // 1) only data[0] and data[1] are non NULL and
    // this function merges 2 equal vertices together
    // In this case we just insert 1 point as the average of the 2 vertices
    // 2) All 4 vertices are non NULL and the 2 first points are the end points of the first edge
    // and the 2 last points are the end points of the second edge.
    // In this case we create 2 points by averaging the first 2 point and the last 2 points respectively

    if (!data[2]) {
        // We are in case 1)
        v.isIntersectionPoint = false;
        v.generatedOrigin1 = getVertexOriginFromParentOrigin(*originalVertices[0], *myData);
        v.t = averageParametric_t(0, myData, originalVertices, weights);
    } else {
        // Case 2)
        v.isIntersectionPoint = true;
        v.generatedOrigin1 = getVertexOriginFromParentOrigin(*originalVertices[0], *myData);
        v.generatedOrigin2 = getVertexOriginFromParentOrigin(*originalVertices[2], *myData);

        assert(v.generatedOrigin1 != eVertexPointsSetGeneratedPoints);
        assert(v.generatedOrigin2 != eVertexPointsSetGeneratedPoints);

        v.contourIndex2 = originalVertices[2]->contourIndex;

        v.t = averageParametric_t(0, myData, originalVertices, weights);
        v.t2 = averageParametric_t(2, myData, originalVertices, weights);
    }
    
    v.index->contourIndex = originalVertices[0]->contourIndex;

    // The original point is not a generated point
    assert(v.generatedOrigin1 != eVertexPointsSetGeneratedPoints);

    myData->generatedPoints.push_back(v);


} // tess_intersection_combine_callback


static Point
getPointFromTriangulation(const PolygonCSGData& inArgs, const VertexIndex& index)
{
    Point p;
    switch (index.origin) {
        case eVertexPointsSetFeather:
            assert(index.pointIndex < (int)inArgs.featherPolygon.size());
            p.x = inArgs.featherPolygon[index.pointIndex].x;
            p.y = inArgs.featherPolygon[index.pointIndex].y;
            break;
            
        case eVertexPointsSetInternalShape:
            assert(index.pointIndex < (int)inArgs.bezierPolygon.size());
            p.x = inArgs.bezierPolygon[index.pointIndex].x;
            p.y = inArgs.bezierPolygon[index.pointIndex].y;
            break;
            
        case eVertexPointsSetGeneratedPoints:
            assert(index.pointIndex < (int)inArgs.generatedPoints.size());
            p.x = inArgs.generatedPoints[index.pointIndex].x;
            p.y = inArgs.generatedPoints[index.pointIndex].y;
            break;
            
    }
    return p;
}



struct ContourBezierVertex
{
    double x,y,t;

    // Are we a point on the contour?
    bool isOnContour;

    // Was this an intersection created by the combined callback ?
    // If true, the t2 member is set to the parametric t of the point created
    // for the second edge
    bool isIntersectionPoint;

    double t2;

    VertexIndexPtr index;

    ContourBezierVertex()
    : x(0)
    , y(0)
    , t(0)
    , isOnContour(false)
    , isIntersectionPoint(false)
    , t2(0)
    {

    }
};

struct ContourCSGData
{

    std::vector<ContourBezierVertex> polygon;

    std::vector< std::vector<VertexIndex> > contourList;

    std::vector<ContourBezierVertex> generatedPoints;

};

static void contour_begin_primitive_callback(unsigned int which,
                                             void *polygonData)
{
    (void)which;
    assert(which == LIBTESS_GL_LINE_LOOP);
    ContourCSGData* myData = (ContourCSGData*)polygonData;
    assert(myData);
    myData->contourList.push_back(std::vector<VertexIndex>());

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
    VertexIndex* index = (VertexIndex*)data;
    myData->contourList.back().push_back(*index);
    
    ContourBezierVertex* from = 0;
    switch (index->origin) {
        case eVertexPointsSetFeather:
        case eVertexPointsSetInternalShape:
            // We process the feather polygon and internal polygon with 2 different calls to ensurePolygonWindingNumberEqualsOne()
            // So it does not matter
            assert(index->pointIndex < (int)myData->polygon.size());
            from = &myData->polygon[index->pointIndex];
            break;
            
        case eVertexPointsSetGeneratedPoints:
            assert(index->pointIndex < (int)myData->generatedPoints.size());
            from = &myData->generatedPoints[index->pointIndex];
            break;
    }
    
    from->isOnContour = true;
}

static void contour_error_callback(unsigned int /*error*/,
                                   void */*polygonData*/)
{
    assert(false);
}

static double averageContourParametric_t(int startIdx, ContourCSGData* myData, VertexIndex* originalVertices[4], float weights[4])
{
    double t = 0.;
    double weightsSum = 0.;
    for (int i = startIdx; i < startIdx + 2; ++i) {
        if (originalVertices[i]) {

            weightsSum += weights[i];

            double vertexT;
            switch (originalVertices[i]->origin) {
                case eVertexPointsSetInternalShape:
                case eVertexPointsSetFeather:
                    assert(originalVertices[i]->pointIndex < (int)myData->polygon.size());
                    vertexT = myData->polygon[originalVertices[i]->pointIndex].t;
                    break;

                case eVertexPointsSetGeneratedPoints:
                    assert(originalVertices[i]->pointIndex < (int)myData->generatedPoints.size());
                    vertexT = myData->generatedPoints[originalVertices[i]->pointIndex].t;
                    break;
            }
            t += (weights[i] * vertexT);
        }
    }
    if (weightsSum > 0) {
        t /= weightsSum;
    }
    return t;
}

static void contour_intersection_combine_callback(double coords[3],
                                                  void *data[4] /*4 original vertices*/,
                                                  float weights[4] /*weights*/,
                                                  void **dataOut,
                                                  void *polygonData)
{
    ContourCSGData* myData = (ContourCSGData*)polygonData;
    assert(myData);

    ContourBezierVertex v;
    v.x = coords[0];
    v.y = coords[1];
    v.isOnContour = false;

    // To find out the parametric t of the generated point, we use the weights on the original vertices
    VertexIndex* originalVertices[4] = {(VertexIndex*)data[0], (VertexIndex*)data[1], (VertexIndex*)data[2], (VertexIndex*)data[3]};


    // There are 2 cases for which this function is called,
    // 1) only data[0] and data[1] are non NULL and
    // this function merges 2 equal vertices together
    // In this case we just insert 1 point as the average of the 2 vertices
    // 2) All 4 vertices are non NULL and the 2 first points are the end points of the first edge
    // and the 2 last points are the end points of the second edge.
    // In this case we create 2 points by averaging the first 2 point and the last 2 points respectively

    if (!data[2]) {
        // We are in case 1)
        v.isIntersectionPoint = false;
        v.t = averageContourParametric_t(0, myData, originalVertices, weights);
    } else {
        // Case 2)
        v.isIntersectionPoint = true;
        v.t = averageContourParametric_t(0, myData, originalVertices, weights);
        v.t2 = averageContourParametric_t(2, myData, originalVertices, weights);
    }

    v.index.reset(new VertexIndex);
    v.index->origin = eVertexPointsSetGeneratedPoints;
    v.index->pointIndex = myData->generatedPoints.size();
    *dataOut = (void*)v.index.get();


    myData->generatedPoints.push_back(v);


} // contour_intersection_combine_callback



static bool contourBezierVertexCompare(const ContourBezierVertex& lhs, const ContourBezierVertex& rhs)
{
    return lhs.t < rhs.t;
}

/**
 * @brief Using libtess, extracts the contour of the polygon so that the winding number of the polygon is one.
 * E.g: we may have a polygon that is self intersecting, in which case this would fail the polygon intersection done
 * in computeInternalPolygon()
 **/
static void ensurePolygonWindingNumberEqualsOne(std::vector<PolygonVertex>& originalPolygon, bool clockWise, VertexPointsSetEnum polygonType, std::vector< std::vector< PolygonVertex > >* contours)
{

    ContourCSGData data;

    data.polygon.resize(originalPolygon.size());
    for (std::size_t i = 0; i < originalPolygon.size(); ++i) {
        data.polygon[i].x = originalPolygon[i].x;
        data.polygon[i].y = originalPolygon[i].y;
        data.polygon[i].t = originalPolygon[i].t;

        // Initialize all points as not being on the contour and then the libtess vertex callback will inform us of points on the contour
        data.polygon[i].isOnContour = false;

        data.polygon[i].index.reset(new VertexIndex);
        data.polygon[i].index->origin = eVertexPointsSetInternalShape;
        data.polygon[i].index->pointIndex = i;
    }



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

    {
        libtess_gluTessBeginContour(tesselator);

        for (std::size_t i = 0; i < data.polygon.size(); ++i) {
            double coords[3] = {data.polygon[i].x, data.polygon[i].y, 1.};
            libtess_gluTessVertex(tesselator, coords, (void*)data.polygon[i].index.get() /*per-vertex client data*/);
        }

        libtess_gluTessEndContour(tesselator);
    }


    libtess_gluTessEndPolygon(tesselator);
    libtess_gluDeleteTess(tesselator);

#if 1
    // Insert all generated points in the polygon at their appropriate location in the polygon, according to their t
    // Since the polygon vector is already sorted by increasing t, this is very fast

    for (std::size_t i = 0; i < data.generatedPoints.size(); ++i) {

        if (!data.generatedPoints[i].isOnContour) {
            continue;
        }


        // lb points to the first vertex in the polygon with a parametric t greater or equal to the generated point t
        {
            std::vector<ContourBezierVertex>::iterator lb = std::lower_bound(data.polygon.begin(), data.polygon.end(), data.generatedPoints[i], contourBezierVertexCompare);
            // Insert it before the first element that is greater
            data.polygon.insert(lb, data.generatedPoints[i]);
        }

        // This point may be an intersection point and might need to be inserted twice in the polygon.
        if (data.generatedPoints[i].isIntersectionPoint) {
            // Swap t and t2 so that the contourBezierVertexCompare function finds the lower bound for t2 correctly
            std::swap(data.generatedPoints[i].t,data.generatedPoints[i].t2);

            std::vector<ContourBezierVertex>::iterator lb = std::lower_bound(data.polygon.begin(), data.polygon.end(), data.generatedPoints[i], contourBezierVertexCompare);
            // Insert it before the first element that is greater
            data.polygon.insert(lb, data.generatedPoints[i]);
        }
    }


    // Update the original polygon in output and remove all points that are not on the contour
    for (std::size_t i = 0; i < data.polygon.size(); ++i) {

        const ContourBezierVertex& from = data.polygon[i];
        if (!from.isOnContour) {
            continue;
        }

        PolygonVertex v;
        v.x = from.x;
        v.y = from.y;
        v.t = from.t;
        v.index.reset(new VertexIndex);
        v.index->origin = polygonType;
        v.index->pointIndex = originalPolygon.size();
        v.isInner = false;
        originalPolygon.push_back(v);
    }
#endif


    contours->resize(data.contourList.size());

    for (std::size_t i = 0; i < data.contourList.size(); ++i) {

        (*contours)[i].resize(data.contourList[i].size());
        for (std::size_t j = 0; j < data.contourList[i].size(); ++j) {

            const VertexIndex& index = data.contourList[i][j];
            ContourBezierVertex* from = 0;
            switch (index.origin) {
                case eVertexPointsSetFeather:
                case eVertexPointsSetInternalShape:
                    // We process the feather polygon and internal polygon with 2 different calls to ensurePolygonWindingNumberEqualsOne()
                    // So it does not matter
                    assert(index.pointIndex < (int)data.polygon.size());
                    from = &data.polygon[index.pointIndex];
                    break;

                case eVertexPointsSetGeneratedPoints:
                    assert(index.pointIndex < (int)data.generatedPoints.size());
                    from = &data.generatedPoints[index.pointIndex];
                    break;
            }

            PolygonVertex &to = (*contours)[i][j];
            to.x = from->x;
            to.y = from->y;
            to.t = from->t;
            to.index.reset(new VertexIndex);
            to.index->contourIndex = i;
            to.index->origin = polygonType;
            to.index->pointIndex = j;
            to.isInner = false;

        }
    }




} // ensurePolygonWindingNumberEqualsOne

inline bool polygonVertexCompare(const PolygonVertex& lhs, const PolygonVertex& rhs)
{
    return lhs.t < rhs.t;
}

static void insertGeneratedPoint(const PolygonVertex& generatedPoint,  PolygonCSGData& data)
{

    std::vector<PolygonVertex>* polygon = 0;
    switch (generatedPoint.generatedOrigin1) {
        case eVertexPointsSetFeather:
            polygon = &data.featherProcessContours[generatedPoint.index->contourIndex];
            break;
        case eVertexPointsSetInternalShape:
            polygon = &data.bezierProcessedContours[generatedPoint.index->contourIndex];
            break;
        case eVertexPointsSetGeneratedPoints:
            // A point is only generated on one of the original polygons
            assert(false);
            break;
    }
    // lb points to the first vertex in the polygon with a parametric t greater or equal to the generated point t
    std::vector<PolygonVertex>::iterator lb = std::lower_bound(polygon->begin(), polygon->end(), generatedPoint, polygonVertexCompare);
    // Insert it before the first element that is greater
    polygon->insert(lb, generatedPoint);
}

/**
 * @brief Using libtess, computes the intersection of the feather polygon and the internal shape polygon so we find the actual interior and exterior of the shape
 **/
static void computeInternalPolygon(PolygonCSGData& data, bool clockWise, RotoBezierTriangulation::PolygonData* outArgs)
{
    
    assert(!data.bezierPolygon.empty() && !data.featherPolygon.empty());

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

    {

        for (std::size_t i = 0; i < data.bezierProcessedContours.size(); ++i) {
            libtess_gluTessBeginContour(tesselator);

            for (std::size_t j = 0; j < data.bezierProcessedContours[i].size(); ++j) {
                assert(data.bezierProcessedContours[i][j].index->origin == eVertexPointsSetInternalShape);
                assert(data.bezierProcessedContours[i][j].index->pointIndex == (int)j);
                assert(data.bezierProcessedContours[i][j].index->contourIndex == (int)i);
                double coords[3] = {data.bezierProcessedContours[i][j].x, data.bezierProcessedContours[i][j].y, 1.};
                libtess_gluTessVertex(tesselator, coords, (void*)data.bezierProcessedContours[i][j].index.get() /*per-vertex client data*/);
            }

            libtess_gluTessEndContour(tesselator);

        }

    }
    {

        for (std::size_t i = 0; i < data.featherProcessContours.size(); ++i) {
            libtess_gluTessBeginContour(tesselator);

            for (std::size_t j = 0; j < data.featherProcessContours[i].size(); ++j) {
                assert(data.featherProcessContours[i][j].index->origin == eVertexPointsSetFeather);
                assert(data.featherProcessContours[i][j].index->pointIndex == (int)j);
                assert(data.featherProcessContours[i][j].index->contourIndex == (int)i);
                double coords[3] = {data.featherProcessContours[i][j].x, data.featherProcessContours[i][j].y, 1.};
                libtess_gluTessVertex(tesselator, coords, (void*)data.featherProcessContours[i][j].index.get() /*per-vertex client data*/);
            }

            libtess_gluTessEndContour(tesselator);
            
        }
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
#ifndef NDEBUG
            data.bezierBbox.contains(outArgs->internalShapeVertices[i].x, outArgs->internalShapeVertices[i].y);
#endif
        }
    }
    outArgs->internalShapeTriangleFans.resize(data.internalFans.size());
    outArgs->internalShapeTriangles.resize(data.internalTriangles.size());
    outArgs->internalShapeTriangleStrips.resize(data.internalStrips.size());

    // Copy triangles
    for (std::size_t i = 0; i < data.internalTriangles.size(); ++i) {
        outArgs->internalShapeTriangles[i].resize(data.internalTriangles[i].size());
        for (std::size_t j = 0; j < data.internalTriangles[i].size(); ++j) {
            const VertexIndex& index = data.internalTriangles[i][j];

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
            const VertexIndex& index = data.internalFans[i][j];

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
            const VertexIndex& index = data.internalStrips[i][j];

            // Find the vertex in the unique set to retrieve its index
            VertexIndexSet::iterator found = data.internalShapeVertices.find(index);
            assert(found != data.internalShapeVertices.end());
            if (found != data.internalShapeVertices.end()) {
                std::size_t vertexIndexInVec = std::distance(data.internalShapeVertices.begin(), found);
                outArgs->internalShapeTriangleStrips[i][j] = vertexIndexInVec;
            }

        }
    }

    // Insert the points generated by libtess for each intersection to their respective polygons to compute the feather
    // Since the polygon vector is already sorted by increasing t, this is very fast
    for (std::size_t i = 0; i < data.generatedPoints.size(); ++i) {

        insertGeneratedPoint(data.generatedPoints[i], data);

        // This point may be an intersection point and might need to be inserted twice in the polygon.
        if (data.generatedPoints[i].isIntersectionPoint) {
            // Swap t and t2 so that the contourBezierVertexCompare function finds the lower bound for t2 correctly
            std::swap(data.generatedPoints[i].t,data.generatedPoints[i].t2);
            std::swap(data.generatedPoints[i].index->contourIndex,data.generatedPoints[i].contourIndex2);
            insertGeneratedPoint(data.generatedPoints[i], data);
        }
    }
} // computeInternalPolygon

inline void polygonPointToBezierVertex(const PolygonVertex& from, RotoBezierTriangulation::BezierVertex* to)
{
    to->x = from.x;
    to->y = from.y;
    to->isInner = from.isInner;
}

/**
 * @brief Using the feather polygon and internal shape polygon and assuming they have the same number of points, iterate through each 
 * point to create a mesh composed of triangles.
 **/
static void computeFeatherTriangles(PolygonCSGData &data, RotoBezierTriangulation::PolygonData* outArgs)
{

    // The discretized bezier polygon and feather polygon must have the same number of samples.
    assert( !data.featherPolygon.empty() && !data.bezierPolygon.empty());

    // Points to the current feather bezier segment
    vector<PolygonVertex>::const_iterator fit = data.featherPolygon.end();
    --fit;
    vector<PolygonVertex> ::const_iterator it = data.bezierPolygon.end();
    --it;

    // Initialize the state with a segment between the first inner vertex and first outter vertex
    RotoBezierTriangulation::BezierVertex lastInnerVert,lastOutterVert;
    polygonPointToBezierVertex(*it, &lastInnerVert);
    polygonPointToBezierVertex(*fit, &lastOutterVert);


    // Whether or not to increment the iterators at the start of the loop
    bool incrFeatherIt = true;
    bool incrBezierIt = true;

    // Initialize the first segment
    outArgs->featherTriangles.push_back(lastOutterVert);
    outArgs->featherTriangles.push_back(lastInnerVert);

    fit = data.featherPolygon.begin();
    it = data.bezierPolygon.begin();

    for(;;) {

        double inner_t = std::numeric_limits<double>::infinity();
        double outter_t = std::numeric_limits<double>::infinity();
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
                polygonPointToBezierVertex(*it, &lastInnerVert);
                outArgs->featherTriangles.push_back(lastInnerVert);
            }
        } else {
            incrBezierIt = false;
            incrFeatherIt = true;
            if ( fit != data.featherPolygon.end() ) {
                polygonPointToBezierVertex(*fit, &lastOutterVert);
                outArgs->featherTriangles.push_back(lastOutterVert);
            }
        }

        if (fit == data.featherPolygon.end() && it == data.bezierPolygon.end()) {
            break;
        }

        // Initialize the first segment
        outArgs->featherTriangles.push_back(lastOutterVert);
        outArgs->featherTriangles.push_back(lastInnerVert);


        if ( incrBezierIt && it != data.bezierPolygon.end() ) {
            ++it;
        }
        if ( incrFeatherIt && fit != data.featherPolygon.end() ) {
            ++fit;
        }
    }


} // computeFeatherTriangles



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
        bool skipFirstPoint = true;//i > 0;
        std::size_t j = skipFirstPoint ? 1 : 0;

        for (; j < bezierPolygon[i].size(); ++j) {
            const ParametricPoint& from = bezierPolygon[i][j];
            PolygonVertex to;
            to.x = from.x;
            to.y = from.y;
            // Add the index of the segment to the parametric t so further in computefeatherTriangles we do not compare 2 points
            // which belong to 2 different segments
            to.t = from.t + i;
            to.isIntersectionPoint = false;
            to.isInner = false;
            data.bezierPolygon.push_back(to);

        }
    }

    // The feather point must be offset along the normal by the feather distance.
    for (std::size_t i = 0; i < featherPolygon.size(); ++i) {

        // Skip the first point in input because it is the same as the last point of the previous segment
        bool skipFirstPoint = true;//i > 0;
        std::size_t startIndex = skipFirstPoint ? 1 : 0;


        for (std::size_t j = startIndex; j < featherPolygon[i].size(); ++j) {

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
                    next = &featherPolygon[i + 1][startIndex];
                } else {
                    next = &featherPolygon[0][startIndex];
                }
            }

            const ParametricPoint& from = featherPolygon[i][j];
            PolygonVertex to;
            to.x = from.x;
            to.y = from.y;
            to.isIntersectionPoint = false;
            // Add the index of the segment to the parametric t so further in computefeatherTriangles we do not compare 2 points
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
        } // for each point in a segment
    } // for each feather bezier segment
} // initializePolygons

NATRON_NAMESPACE_ANONYMOUS_EXIT;


void
RotoBezierTriangulation::tesselate(const BezierPtr& bezier,
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

    PolygonCSGData data;
    data.outArgs = outArgs;

    std::vector<std::vector<ParametricPoint> > featherPolygonOrig;
    std::vector<std::vector<ParametricPoint> > bezierPolygonOrig;

    bezier->evaluateFeatherPointsAtTime(time, view, scale, Bezier::eDeCastelJauAlgorithmIterative, 10, 1., &featherPolygonOrig, 0,
#ifndef NDEBUG
                                        &data.bezierBbox
#else
                                        0
#endif
                                        );
    bezier->evaluateAtTime(time, view, scale, Bezier::eDeCastelJauAlgorithmIterative, 10, 1., &bezierPolygonOrig, 0, 0);


    // Merge all bezier segments into a single polygon
    initializePolygons(data, clockWise, featherPolygonOrig, bezierPolygonOrig, featherDistPixel_x, featherDistPixel_y);

    // Pre-process the polygons so that their winding number is 1 always so that the polygon intersection can be found
    // later on with libtess
    ensurePolygonWindingNumberEqualsOne(data.featherPolygon, clockWise, eVertexPointsSetFeather, &data.featherProcessContours);
    ensurePolygonWindingNumberEqualsOne(data.bezierPolygon, clockWise, eVertexPointsSetInternalShape, &data.bezierProcessedContours);
    
    // Compute the intersection of both polygons to extract the inner shape
    computeInternalPolygon(data, clockWise, outArgs);

    // Now that we have the role (inner or outter) for each vertex, compute the feather mesh
    computeFeatherTriangles(data, outArgs);

} // tesselate



NATRON_NAMESPACE_EXIT;
