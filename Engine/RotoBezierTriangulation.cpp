/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2020 The Natron developers
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


NATRON_NAMESPACE_ENTER


NATRON_NAMESPACE_ANONYMOUS_ENTER

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




struct VertexIndexCompare {
    bool operator() (const VertexIndex& lhs, const VertexIndex& rhs) const
    {
        if (lhs.origin < rhs.origin) {
            return true;
        } else if (lhs.origin > rhs.origin) {
            return false;
        }

        return lhs.pointIndex < rhs.pointIndex;
    }
};

typedef std::set<VertexIndex, VertexIndexCompare> VertexIndexSet;
typedef std::vector<VertexIndex> VertexIndexVector;

struct PolygonVertex
{

    // The index in the modified polygon
    VertexIndexPtr index;

    // The index in the original polygon
    VertexIndex originalIndex;

    double x,y,t;
    bool isInner;

    // Was this an intersection created by the combined callback ?
    // If true, the t2 member is set to the parametric t of the point created
    // for the second edge.
    // Similarly, generatedOrigin2 is the vertex origin if it is an intersection
    bool isIntersectionPoint;
    double t2;


};

struct PolygonCSGData
{

    RotoBezierTriangulation::PolygonData* outArgs;
    
    // The polygon of the Bezier and feather with parametric t and flag indicating the color of each point
    std::vector<PolygonVertex> originalBezierPolygon, originalFeatherPolygon, modifiedBezierPolygon, modifiedFeatherPolygon;

    // The bezierPolygon and featherPolygon contour(s) in output of ensurePolygonWindingNumberEqualsOne
    std::vector<std::vector<VertexIndex> > bezierProcessedContours, featherProcessContours;
    
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
        case eVertexPointsSetFeather: {
            assert(index->pointIndex < (int)myData->modifiedFeatherPolygon.size());
            myData->modifiedFeatherPolygon[index->pointIndex].isInner = true;
            const VertexIndex& originalIndex = myData->modifiedFeatherPolygon[index->pointIndex].originalIndex;
            assert(originalIndex.pointIndex < (int)myData->originalFeatherPolygon.size());
            myData->originalFeatherPolygon[originalIndex.pointIndex].isInner = true;
        }   break;
        case eVertexPointsSetInternalShape: {
            assert(index->pointIndex < (int)myData->modifiedBezierPolygon.size());
            myData->modifiedBezierPolygon[index->pointIndex].isInner = true;
            const VertexIndex& originalIndex = myData->modifiedBezierPolygon[index->pointIndex].originalIndex;
            assert(originalIndex.pointIndex < (int)myData->originalBezierPolygon.size());
            myData->originalBezierPolygon[originalIndex.pointIndex].isInner = true;
        }   break;
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
                    assert(originalVertices[i]->pointIndex < (int)myData->modifiedBezierPolygon.size());
                    vertexT = myData->modifiedBezierPolygon[originalVertices[i]->pointIndex].t;
                    break;
                case eVertexPointsSetFeather:

                    assert(originalVertices[i]->pointIndex < (int)myData->modifiedFeatherPolygon.size());
                    vertexT = myData->modifiedFeatherPolygon[originalVertices[i]->pointIndex].t;

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

static void tess_intersection_combine_callback(double coords[3],
                                               void *data[4] /*4 original vertices*/,
                                               float weights[4] /*weights*/,
                                               void **dataOut,
                                               void *polygonData)
{
    // We are at an intersection of the feather and internal Bezier.
    // We add a point to the Bezier polygon.
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
        v.t = averageParametric_t(0, myData, originalVertices, weights);
    } else {
        // Case 2)
        v.isIntersectionPoint = true;

        v.t = averageParametric_t(0, myData, originalVertices, weights);
        v.t2 = averageParametric_t(2, myData, originalVertices, weights);
    }

    myData->generatedPoints.push_back(v);


} // tess_intersection_combine_callback


static Point
getPointFromTriangulation(const PolygonCSGData& inArgs, const VertexIndex& index)
{
    Point p;
    switch (index.origin) {
        case eVertexPointsSetFeather:
            assert(index.pointIndex < (int)inArgs.modifiedFeatherPolygon.size());
            p.x = inArgs.modifiedFeatherPolygon[index.pointIndex].x;
            p.y = inArgs.modifiedFeatherPolygon[index.pointIndex].y;
            break;
            
        case eVertexPointsSetInternalShape:
            assert(index.pointIndex < (int)inArgs.modifiedBezierPolygon.size());
            p.x = inArgs.modifiedBezierPolygon[index.pointIndex].x;
            p.y = inArgs.modifiedBezierPolygon[index.pointIndex].y;
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

    std::vector<std::vector<VertexIndex> > contourList;

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



struct ContourBezierVertexWithIndex
{
    ContourBezierVertex v;
    VertexIndex index;
};

inline bool contourBezierVertexCompare(const ContourBezierVertexWithIndex& lhs, const ContourBezierVertexWithIndex& rhs)
{
    return lhs.v.t < rhs.v.t;
}

typedef std::map<VertexIndex, VertexIndex, VertexIndexCompare> VertexIndicesMap;

static void insertGeneratedPoint(std::vector<ContourBezierVertexWithIndex>& polygon,
                                 VertexIndicesMap &oldToNewIndexMap,
                                 const ContourBezierVertexWithIndex& generated)
{
    std::vector<ContourBezierVertexWithIndex>::iterator lb = std::lower_bound(polygon.begin(), polygon.end(), generated, contourBezierVertexCompare);



    // Insert it before the first element that is greater
    lb = polygon.insert(lb, generated);

    // lb points now to the point we just inserted


    // Increment the index of all points after the insertion point
    std::size_t startIndex = std::distance(polygon.begin(), lb);
    for (std::size_t j = startIndex; j < polygon.size(); ++j) {
        // Check if the VertexIndex has already been modified, to retrieve the real
        // original vertex
        VertexIndex oldIndex = *polygon[j].v.index;
        polygon[j].index.pointIndex = j;
        oldToNewIndexMap[oldIndex] = polygon[j].index;

    }
} // insertGeneratedPoint

/**
 * @brief Using libtess, extracts the contour of the polygon so that the winding number of the polygon is one.
 * E.g: we may have a polygon that is self intersecting, in which case this would fail the polygon intersection done
 * in computeInternalPolygon()
 **/
static void ensurePolygonWindingNumberEqualsOne(const PolygonCSGData& mainData,
                                                std::vector<PolygonVertex>& originalPolygon,
                                                std::vector<PolygonVertex>& modifiedPolygon,
                                                bool clockWise,
                                                VertexPointsSetEnum polygonType,
                                                std::vector<std::vector<VertexIndex > >* contours)
{

    (void)mainData;

    ContourCSGData data;

    data.polygon.resize(originalPolygon.size());
    for (std::size_t i = 0; i < originalPolygon.size(); ++i) {
        data.polygon[i].x = originalPolygon[i].x;
        data.polygon[i].y = originalPolygon[i].y;
        data.polygon[i].t = originalPolygon[i].t;
        assert(mainData.bezierBbox.contains(data.polygon[i].x, data.polygon[i].y));

        // Initialize all points as not being on the contour and then the libtess vertex callback will inform us of points on the contour
        data.polygon[i].isOnContour = false;

        data.polygon[i].index.reset(new VertexIndex);
        data.polygon[i].index->origin = eVertexPointsSetInternalShape;
        data.polygon[i].index->pointIndex = i;

        originalPolygon[i].index.reset(new VertexIndex);
        originalPolygon[i].index->origin = polygonType;
        originalPolygon[i].index->pointIndex = i;
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
            assert(mainData.bezierBbox.contains(coords[0], coords[1]));
            libtess_gluTessVertex(tesselator, coords, (void*)data.polygon[i].index.get() /*per-vertex client data*/);
        }

        libtess_gluTessEndContour(tesselator);
    }


    libtess_gluTessEndPolygon(tesselator);
    libtess_gluDeleteTess(tesselator);

    // Clear the original polygon,  we are going to modify it
    modifiedPolygon.clear();

    // Insert all generated points in the polygon at their appropriate location in the polygon, according to their t
    // Since the polygon vector is already sorted by increasing t, this is very fast
    VertexIndicesMap oldToNewIndexMap;

    // Since we are going to insert and remove points in the polygon, we need to update the indices and keep a mapping
    // so that the vertex indices in data.contourList still point to a valid index.
    std::vector<ContourBezierVertexWithIndex> polygonCopy(data.polygon.size());
    for (std::size_t i = 0; i < data.polygon.size(); ++i) {
        polygonCopy[i].v = data.polygon[i];
        polygonCopy[i].index = *data.polygon[i].index;
        oldToNewIndexMap[polygonCopy[i].index] = polygonCopy[i].index;
    }
    for (std::size_t i = 0; i < data.generatedPoints.size(); ++i) {

        if (!data.generatedPoints[i].isOnContour) {
            continue;
        }
        assert(mainData.bezierBbox.contains(data.generatedPoints[i].x, data.generatedPoints[i].y));

        {
            ContourBezierVertexWithIndex wrapper;
            wrapper.v = data.generatedPoints[i];
            wrapper.index = *wrapper.v.index;
            insertGeneratedPoint(polygonCopy, oldToNewIndexMap, wrapper);
        }

        // This point may be an intersection point and might need to be inserted twice in the polygon.
        if (data.generatedPoints[i].isIntersectionPoint) {
            // Swap t and t2 so that the contourBezierVertexCompare function finds the lower bound for t2 correctly
            std::swap(data.generatedPoints[i].t,data.generatedPoints[i].t2);

            {
                ContourBezierVertexWithIndex wrapper;
                wrapper.v = data.generatedPoints[i];
                wrapper.index = *wrapper.v.index;
                insertGeneratedPoint(polygonCopy, oldToNewIndexMap, wrapper);
            }
        }
    }

#ifdef DEBUG
    // Check that all t and indices are increasing
    for (std::size_t i = 1; i < polygonCopy.size(); ++i) {
        assert(polygonCopy[i].v.t > polygonCopy[i - 1].v.t);
        assert(polygonCopy[i].index.pointIndex == polygonCopy[i - 1].index.pointIndex + 1);
    }
#endif

    // Update the original polygon in output and remove all points that are not on the contour
    for (std::size_t i = 0; i < polygonCopy.size(); ++i) {
        const ContourBezierVertexWithIndex& from = polygonCopy[i];
        
        if (!from.v.isOnContour) {

            // Decrement the index of all points after this one since we removed it
            for (std::size_t j = i + 1; j < polygonCopy.size(); ++j) {

                // Check if the VertexIndex has already been modified, to retrieve the real
                // original vertex
                VertexIndex oldIndex = *polygonCopy[j].v.index;
                --polygonCopy[j].index.pointIndex;

                // Record the indices changes
                oldToNewIndexMap[oldIndex] = polygonCopy[j].index;
            }
        } else {

            PolygonVertex v;
            v.x = from.v.x;
            v.y = from.v.y;
            v.t = from.v.t;

            // The index should be up to date
            assert(from.index.pointIndex == (int)modifiedPolygon.size());
            v.index.reset(new VertexIndex);
            v.index->origin = polygonType;
            v.index->pointIndex = modifiedPolygon.size();
            v.originalIndex = *from.v.index;
            assert(v.originalIndex.pointIndex < (int)originalPolygon.size());
            v.isInner = false;
            modifiedPolygon.push_back(v);
        } // isOnContour
    }



#ifdef DEBUG
    // Check that all indices are increasing in the new polygon
    for (std::size_t i = 1; i < modifiedPolygon.size(); ++i) {
        assert(modifiedPolygon[i].index->pointIndex == modifiedPolygon[i - 1].index->pointIndex + 1);
    }
#endif

    // Finally copy the contours in output, and update the indices that still point to old polygon
    // indices to the modified polygon indices using the indicesMap
    contours->resize(data.contourList.size());

    for (std::size_t i = 0; i < data.contourList.size(); ++i) {

        (*contours)[i].resize(data.contourList[i].size());
        for (std::size_t j = 0; j < data.contourList[i].size(); ++j) {

            const VertexIndex& originalIndex = data.contourList[i][j];

            // Find the corresponding index in the indicesMap since we modified the indices
            VertexIndicesMap::iterator foundMapping = oldToNewIndexMap.find(originalIndex);
            assert(foundMapping != oldToNewIndexMap.end());
            const VertexIndex& mappedIndex = foundMapping->second;

            VertexIndex &to = (*contours)[i][j];
            to.pointIndex = mappedIndex.pointIndex;
            to.origin = polygonType;

            assert(to.pointIndex < (int)modifiedPolygon.size());
        }
    }


} // ensurePolygonWindingNumberEqualsOne


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

    {

        for (std::size_t i = 0; i < data.bezierProcessedContours.size(); ++i) {
            libtess_gluTessBeginContour(tesselator);

            for (std::size_t j = 0; j < data.bezierProcessedContours[i].size(); ++j) {
                assert(data.bezierProcessedContours[i][j].origin == eVertexPointsSetInternalShape);
                assert(data.bezierProcessedContours[i][j].pointIndex < (int)data.modifiedBezierPolygon.size());
                const PolygonVertex& vertex = data.modifiedBezierPolygon[data.bezierProcessedContours[i][j].pointIndex];
                double coords[3] = {vertex.x, vertex.y, 1.};
                assert(vertex.index);
                libtess_gluTessVertex(tesselator, coords, (void*)vertex.index.get() /*per-vertex client data*/);
            }

            libtess_gluTessEndContour(tesselator);

        }

    }
    {

        for (std::size_t i = 0; i < data.featherProcessContours.size(); ++i) {
            libtess_gluTessBeginContour(tesselator);

            for (std::size_t j = 0; j < data.featherProcessContours[i].size(); ++j) {
                assert(data.featherProcessContours[i][j].origin == eVertexPointsSetFeather);
                assert(data.featherProcessContours[i][j].pointIndex < (int)data.modifiedFeatherPolygon.size());
                const PolygonVertex& vertex = data.modifiedFeatherPolygon[data.featherProcessContours[i][j].pointIndex];
                double coords[3] = {vertex.x, vertex.y, 1.};
                assert(vertex.index);
                libtess_gluTessVertex(tesselator, coords, (void*)vertex.index.get() /*per-vertex client data*/);

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
            assert(data.bezierBbox.contains(outArgs->internalShapeVertices[i].x, outArgs->internalShapeVertices[i].y));
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


} // computeInternalPolygon


/**
 * @brief Using the feather polygon and internal shape polygon and assuming they have the same number of points, iterate through each 
 * point to create a mesh composed of triangles.
 **/
static void computeFeatherTriangles(PolygonCSGData &data, RotoBezierTriangulation::PolygonData* outArgs)
{

    // The discretized Bezier polygon and feather polygon must have the same number of samples.
    assert( !data.originalFeatherPolygon.empty() && !data.originalBezierPolygon.empty());

    // In output, each unique feather vertex
    VertexIndexSet featherVertices;
    std::vector<VertexIndex> featherTriangleIndices;

    // Repeat the first point of the polygon to the end so that we close the shape
    data.originalBezierPolygon.push_back(data.originalBezierPolygon.front());
    data.originalFeatherPolygon.push_back(data.originalFeatherPolygon.front());


    // Points to the end of each polygon
    const vector<PolygonVertex>::iterator iteratorEnd[2] = {data.originalBezierPolygon.end(), data.originalFeatherPolygon.end()};
    const vector<PolygonVertex>::iterator iteratorBegin[2] = {data.originalBezierPolygon.begin(), data.originalFeatherPolygon.begin()};

    // inner = 0, outter = 1

    // Points to the current feather Bezier segment
    vector<PolygonVertex>::iterator it[2];
    for (int i = 0; i < 2; ++i) {
        it[i] = iteratorEnd[i];
        --it[i];
    }

#if 0
    // If both polygon have an outter point for starter, skip these points
    {
        bool hasInner = false;

        // True if we passed iteratorEnd, to avoid infinite looping
        bool loopedOnce[2] = {false, false};
        while (!hasInner) {
            for (int i = 0; i < 2; ++i) {
                if (it[i] == iteratorEnd[i]) {
                    if (loopedOnce[i]) {
                        return;
                    }
                    loopedOnce[i] = true;
                    it[i] = iteratorBegin[i];
                }
                if (it[i]->isInner) {
                    hasInner = true;
                }
            }
            if (!hasInner) {
                ++it[0];
                ++it[1];
            }
        }
    }

    if (it[0] == iteratorEnd[0] || it[1] == iteratorEnd[1]) {
        // We never found a inner point, don't draw anything
        return;
    }
#endif

    // The last vertex added to the mesh for each polygon
    VertexIndex lastVertexIndex[2];
    for (int i = 0; i < 2; ++i) {
        lastVertexIndex[i] = *it[i]->index;
    }


    // Tells which polygon had the role of the feather in the last iteration
    int lastIterationInnerPolygon = (it[0] != iteratorEnd[0] && it[0]->isInner) ? 0 : 1;

    // Initialize the first segment with 1 vertex from each polygon
    featherVertices.insert(lastVertexIndex[0]);
    featherVertices.insert(lastVertexIndex[1]);

    featherTriangleIndices.push_back(lastVertexIndex[0]);
    featherTriangleIndices.push_back(lastVertexIndex[1]);

    // Iterator to the last inner point for each polygon
    vector<PolygonVertex>::iterator lastInnerIt[2] = {iteratorEnd[0], iteratorEnd[1]};
    for (int i = 0; i < 2; ++i) {
        if (it[i] != iteratorEnd[i] && it[i]->isInner) {
            lastInnerIt[i] = it[i];
        }
    }


    for (int i = 0; i < 2; ++i) {
        if (it[i] == iteratorEnd[i]) {
            it[i] = iteratorBegin[i];
        } else {
            ++it[i];
            if (it[i] == iteratorEnd[i]) {
                it[i] = iteratorBegin[i];
            }
        }
    }

    for(;;) {

        // The parametric t at this point on both polygons
        double parametric_t[2] = {std::numeric_limits<double>::infinity(), std::numeric_limits<double>::infinity()};

        // Whether each polygon current point is considered inner (full opacity) or outter (black and transparant)
        bool isInner[2] = {false, false};

        for (int i = 0; i < 2; ++i) {
            if (it[i] != iteratorEnd[i]) {
                parametric_t[i] = it[i]->t;
                isInner[i] = it[i]->isInner;
            }
        }


        if (isInner[0] != isInner[1]) {
            // Regular case: remember which polygon was the inner polygon (it is possible that the Bezier polygon becomes the outter polygon if the user applied a negative feather)
            lastIterationInnerPolygon = isInner[0] ? 0 : 1;
        } else if (!isInner[0] && !isInner[1]) {

            // If both the feather and the internal polygon point are marked as being outter, that means most likely the points were not emitted by libtess.
            // However we still need to draw some feather there otherwise this will render black triangles.
            // We continue along the last polygon point to create triangle fans

            // Force to pick the point on the outter polygon and also increment the inner polygon point to skip the points
            // that are not marked inner
            if (it[lastIterationInnerPolygon] != iteratorEnd[lastIterationInnerPolygon]) {
                ++it[lastIterationInnerPolygon];
            }
            if (lastInnerIt[lastIterationInnerPolygon] != iteratorEnd[lastIterationInnerPolygon]) {
                assert(lastInnerIt[lastIterationInnerPolygon]->isInner);
                lastVertexIndex[lastIterationInnerPolygon] = *lastInnerIt[lastIterationInnerPolygon]->index;
            }

            // Add a vertex of the outter polygon
            parametric_t[lastIterationInnerPolygon] = 1;
            parametric_t[(lastIterationInnerPolygon + 1) % 2] = 0;
        }
        
#define ADD_POLY_VERTEX(i) \
    if (isInner[i]) { \
        \
        lastInnerIt[i] = it[i]; \
    } \
    if ( it[i] != iteratorEnd[i] ) { \
        lastVertexIndex[i] = *it[i]->index; \
        featherVertices.insert(lastVertexIndex[i]); \
        featherTriangleIndices.push_back(lastVertexIndex[i]); \
        ++it[i]; \
    }
        // Pick the point with the minimum t
        if (parametric_t[0] <= parametric_t[1]) {
            ADD_POLY_VERTEX(0)
        } else {
            ADD_POLY_VERTEX(1)
        }

        if (it[0] == iteratorEnd[0] && it[1] == iteratorEnd[1]) {
            // We reach the end of both polygons
            break;
        }

        // Initialize the first 2 vertices of the next triangle
        featherTriangleIndices.push_back(lastVertexIndex[0]);
        featherTriangleIndices.push_back(lastVertexIndex[1]);

    } // infinite loop

    // Copy back the indices & vertices to the outArgs
    {
        outArgs->featherVertices.resize(featherVertices.size());
        int i = 0;
        for (VertexIndexSet::const_iterator it2 = featherVertices.begin(); it2 != featherVertices.end(); ++it2, ++i) {
            const VertexIndex& index = *it2;
            const PolygonVertex* from = 0;
            switch (it2->origin) {
                case eVertexPointsSetFeather:
                    assert(index.pointIndex < (int)data.originalFeatherPolygon.size());
                    from = &data.originalFeatherPolygon[index.pointIndex];
                    break;

                case eVertexPointsSetInternalShape:
                    assert(index.pointIndex < (int)data.originalBezierPolygon.size());
                    from = &data.originalBezierPolygon[index.pointIndex];
                    break;

                case eVertexPointsSetGeneratedPoints:
                    assert(false);
                    break;
                    
            }
            assert(from);
            RotoBezierTriangulation::BezierVertex &to = outArgs->featherVertices[i];
            to.x = from->x;
            to.y = from->y;
            to.isInner = from->isInner;
            assert(data.bezierBbox.contains(to.x, to.y));

        }
    }
    {
        outArgs->featherTriangles.resize(featherTriangleIndices.size());
        for (std::size_t i = 0; i < featherTriangleIndices.size(); ++i) {
            VertexIndexSet::iterator foundVertex = featherVertices.find(featherTriangleIndices[i]);
            assert(foundVertex != featherVertices.end());
            if (foundVertex != featherVertices.end()) {
                std::size_t index = std::distance(featherVertices.begin(), foundVertex);
                outArgs->featherTriangles[i] = index;
            }
        }
    }
} // computeFeatherTriangles

static void originalPolygonCopy(PolygonCSGData& data,
                                const std::vector<ParametricPoint>& input,
                                std::vector<PolygonVertex>& output)
{
    output.resize(input.size());

    for (std::size_t i = 0; i < input.size(); ++i) {

        const ParametricPoint& from = input[i];
        PolygonVertex &to = output[i];
        to.x = from.x;
        to.y = from.y;
        assert(data.bezierBbox.contains(to.x, to.y));
        // Add the index of the segment to the parametric t so further in computefeatherTriangles we do not compare 2 points
        // which belong to 2 different segments
        to.t = from.t;
        to.isIntersectionPoint = false;
        to.isInner = false;
        assert(data.bezierBbox.contains(to.x, to.y));

        
    }
}


NATRON_NAMESPACE_ANONYMOUS_EXIT


void
RotoBezierTriangulation::tesselate(const BezierPtr& bezier,
                                   TimeValue time,
                                   ViewIdx view,
                                   const RenderScale& scale,
                                   PolygonData* outArgs)
{
    ///Note that we do not use the opacity when rendering the bezier, it is rendered with correct floating point opacity/color when converting
    ///to the Natron image.

    assert(outArgs);

    bool clockWise = bezier->isClockwiseOriented(time, view);

    PolygonCSGData data;
    data.outArgs = outArgs;

    std::vector<ParametricPoint> featherPolygonOrig;
    std::vector<ParametricPoint> bezierPolygonOrig;

#ifndef NDEBUG
    RectD featherBbox;
#endif
    bezier->evaluateFeatherPointsAtTime(true /*applyFeatherDistance*/, time, view, scale, Bezier::eDeCasteljauAlgorithmIterative, -1, 1., &featherPolygonOrig,
#ifndef NDEBUG
                                        &featherBbox
#else
                                        0
#endif
                                        );
    bezier->evaluateAtTime(time, view, scale, Bezier::eDeCasteljauAlgorithmIterative, -1, 1., &bezierPolygonOrig,
#ifndef NDEBUG
                           &data.bezierBbox
#else
                           0
#endif
                           );
#ifndef NDEBUG
    data.bezierBbox.merge(featherBbox);
    /*qDebug() << "tesselate: feather bbox";
    featherBbox.debug();
    qDebug() << "tesselate merge bbox:";
    data.bezierBbox.debug();*/
#endif


    // Copy the feather and Bezier polygon and introduce a isInner flag to determine if a point should be drawn with an inside color (full opacity) or outter
    // color (black)
    // Initialize the isInner flag to false and libtess will inform us of the internal points with the vertex callback

    originalPolygonCopy(data, bezierPolygonOrig, data.originalBezierPolygon);
    originalPolygonCopy(data, featherPolygonOrig, data.originalFeatherPolygon);

    // Pre-process the polygons so that their winding number is 1 always so that the polygon intersection can be found
    // later on with libtess
    ensurePolygonWindingNumberEqualsOne(data, data.originalFeatherPolygon, data.modifiedFeatherPolygon, clockWise, eVertexPointsSetFeather, &data.featherProcessContours);
    ensurePolygonWindingNumberEqualsOne(data, data.originalBezierPolygon, data.modifiedBezierPolygon, clockWise, eVertexPointsSetInternalShape, &data.bezierProcessedContours);
    
    // Compute the intersection of both polygons to extract the inner shape
    computeInternalPolygon(data, clockWise, outArgs);

    // Now that we have the role (inner or outter) for each vertex, compute the feather mesh
    computeFeatherTriangles(data, outArgs);

} // tesselate



NATRON_NAMESPACE_EXIT
