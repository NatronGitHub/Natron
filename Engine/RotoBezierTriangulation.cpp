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

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "RotoBezierTriangulation.h"

#include "libtess.h"

NATRON_NAMESPACE_ENTER;

NATRON_NAMESPACE_ANONYMOUS_ENTER;

static void tess_begin_primitive_callback(unsigned int which, void *polygonData)
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

static void tess_vertex_callback(void* data /*per-vertex client data*/, void *polygonData)
{
    RotoBezierTriangulation::PolygonData* myData = (RotoBezierTriangulation::PolygonData*)polygonData;
    assert(myData);

    assert((myData->stripsBeingEdited && !myData->fanBeingEdited && !myData->trianglesBeingEdited) ||
           (!myData->stripsBeingEdited && myData->fanBeingEdited && !myData->trianglesBeingEdited) ||
           (!myData->stripsBeingEdited && !myData->fanBeingEdited && myData->trianglesBeingEdited));

    unsigned int* p = (unsigned int*)data;
    assert(p);
    assert(/**p >= 0 &&*/ *p < myData->bezierPolygonJoined.size());
#ifndef NDEBUG
    assert(myData->bezierPolygonJoined[*p].x >= myData->bezierBbox.x1 && myData->bezierPolygonJoined[*p].x <= myData->bezierBbox.x2 &&
           myData->bezierPolygonJoined[*p].y >= myData->bezierBbox.y1 && myData->bezierPolygonJoined[*p].y <= myData->bezierBbox.y2);
#endif
    if (myData->stripsBeingEdited) {
        myData->stripsBeingEdited->indices.push_back(*p);
    } else if (myData->fanBeingEdited) {
        myData->fanBeingEdited->indices.push_back(*p);
    } else if (myData->trianglesBeingEdited) {
        myData->trianglesBeingEdited->indices.push_back(*p);
    }

}

static void tess_error_callback(unsigned int error, void *polygonData)
{
    RotoBezierTriangulation::PolygonData* myData = (RotoBezierTriangulation::PolygonData*)polygonData;
    assert(myData);
    myData->error = error;
}

static void tess_intersection_combine_callback(double coords[3], void */*data*/[4] /*4 original vertices*/, double /*w*/[4] /*weights*/, void **dataOut, void *polygonData)
{
    RotoBezierTriangulation::PolygonData* myData = (RotoBezierTriangulation::PolygonData*)polygonData;
    assert(myData);


    ParametricPoint v;
    v.x = coords[0];
    v.y = coords[1];
    v.t = 0;

    assert(myData->bezierPolygonIndices.size() == myData->bezierPolygonJoined.size());

    unsigned int* vertexData =  new unsigned int;
    *vertexData = myData->bezierPolygonJoined.size();

    myData->bezierPolygonIndices.push_back(vertexData);
    myData->bezierPolygonJoined.push_back(v);

    /*new->r = w[0]*d[0]->r + w[1]*d[1]->r + w[2]*d[2]->r + w[3]*d[3]->r;
     new->g = w[0]*d[0]->g + w[1]*d[1]->g + w[2]*d[2]->g + w[3]*d[3]->g;
     new->b = w[0]*d[0]->b + w[1]*d[1]->b + w[2]*d[2]->b + w[3]*d[3]->b;
     new->a = w[0]*d[0]->a + w[1]*d[1]->a + w[2]*d[2]->a + w[3]*d[3]->a;*/
    assert(/**vertexData >= 0 &&*/ *vertexData < myData->bezierPolygonIndices.size());
    *dataOut = (void*)vertexData;
    
}

NATRON_NAMESPACE_ANONYMOUS_EXIT;

void
RotoBezierTriangulation::computeTriangles(const BezierPtr& bezier, double time, ViewIdx view, unsigned int mipmapLevel, double featherDist,
                                             PolygonData* outArgs)
{
    ///Note that we do not use the opacity when rendering the bezier, it is rendered with correct floating point opacity/color when converting
    ///to the Natron image.

    assert(outArgs);
    outArgs->error = 0;

    bool clockWise = bezier->isFeatherPolygonClockwiseOriented(time, view);

    const double absFeatherDist = std::abs(featherDist);

    RectD featherPolyBBox;
    featherPolyBBox.setupInfinity();

#ifdef ROTO_BEZIER_EVAL_ITERATIVE
    int error = -1;
#else
    double error = 1;
#endif

    bezier->evaluateFeatherPointsAtTime_DeCasteljau(time, view, mipmapLevel,error, true, &outArgs->featherPolygon, &featherPolyBBox);
    bezier->evaluateAtTime_DeCasteljau(time, view, mipmapLevel, error,&outArgs->bezierPolygon,
#ifndef NDEBUG
                                       &outArgs->bezierBbox
#else
                                       0
#endif
                                       );


    // First compute the mesh composed of triangles of the feather
    assert( !outArgs->featherPolygon.empty() && !outArgs->bezierPolygon.empty() && outArgs->featherPolygon.size() == outArgs->bezierPolygon.size());

    std::vector<std::vector<ParametricPoint> >::const_iterator fIt = outArgs->featherPolygon.begin();
    std::vector<std::vector<ParametricPoint> > ::const_iterator prevFSegmentIt = outArgs->featherPolygon.end();
    --prevFSegmentIt;
    std::vector<std::vector<ParametricPoint> > ::const_iterator nextFSegmentIt = outArgs->featherPolygon.begin();
    ++nextFSegmentIt;
    for (std::vector<std::vector<ParametricPoint> > ::const_iterator it = outArgs->bezierPolygon.begin(); it != outArgs->bezierPolygon.end(); ++it, ++fIt) {

        if (prevFSegmentIt == outArgs->featherPolygon.end()) {
            prevFSegmentIt = outArgs->featherPolygon.begin();
        }
        if (nextFSegmentIt == outArgs->featherPolygon.end()) {
            nextFSegmentIt = outArgs->featherPolygon.begin();
        }
        // Iterate over each bezier segment.
        // There are the same number of bezier segments for the feather and the internal bezier. Each discretized segment is a contour (list of vertices)

        std::vector<ParametricPoint>::const_iterator bSegmentIt = it->begin();
        std::vector<ParametricPoint>::const_iterator fSegmentIt = fIt->begin();

        assert(!it->empty() && !fIt->empty());


        // prepare iterators to compute derivatives for feather distance
        std::vector<ParametricPoint>::const_iterator fnext = fSegmentIt;
        ++fnext;  // can only be valid since we assert the list is not empty
        if ( fnext == fIt->end() ) {
            fnext = fIt->begin();
        }
        std::vector<ParametricPoint>::const_iterator fprev = prevFSegmentIt->end();
        --fprev; // can only be valid since we assert the list is not empty
        --fprev;


        // initialize the state with a segment between the first inner vertex and first outter vertex
        RotoFeatherVertex lastInnerVert,lastOutterVert;
        if ( bSegmentIt != it->end() ) {
            lastInnerVert.x = bSegmentIt->x;
            lastInnerVert.y = bSegmentIt->y;
            lastInnerVert.isInner = true;
            outArgs->featherMesh.push_back(lastInnerVert);
            ++bSegmentIt;
        }
        if ( fSegmentIt != fIt->end() ) {
            lastOutterVert.x = fSegmentIt->x;
            lastOutterVert.y = fSegmentIt->y;

            if (absFeatherDist) {
                double diffx = fnext->x - fprev->x;
                double diffy = fnext->y - fprev->y;
                double norm = std::sqrt( diffx * diffx + diffy * diffy );
                double dx = (norm != 0) ? -( diffy / norm ) : 0;
                double dy = (norm != 0) ? ( diffx / norm ) : 1;

                if (!clockWise) {
                    lastOutterVert.x -= dx * absFeatherDist;
                    lastOutterVert.y -= dy * absFeatherDist;
                } else {
                    lastOutterVert.x += dx * absFeatherDist;
                    lastOutterVert.y += dy * absFeatherDist;
                }
            }

            lastOutterVert.isInner = false;
            outArgs->featherMesh.push_back(lastOutterVert);
            ++fSegmentIt;
        }

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
                    lastInnerVert.isInner = true;
                    outArgs->featherMesh.push_back(lastInnerVert);
                    ++bSegmentIt;
                }
            } else {
                if ( fSegmentIt != fIt->end() ) {
                    lastOutterVert.x = fSegmentIt->x;
                    lastOutterVert.y = fSegmentIt->y;

                    if (absFeatherDist) {
                        double diffx = fnext->x - fprev->x;
                        double diffy = fnext->y - fprev->y;
                        double norm = std::sqrt( diffx * diffx + diffy * diffy );
                        double dx = (norm != 0) ? -( diffy / norm ) : 0;
                        double dy = (norm != 0) ? ( diffx / norm ) : 1;

                        if (!clockWise) {
                            lastOutterVert.x -= dx * absFeatherDist;
                            lastOutterVert.y -= dy * absFeatherDist;
                        } else {
                            lastOutterVert.x += dx * absFeatherDist;
                            lastOutterVert.y += dy * absFeatherDist;
                        }
                    }
                    lastOutterVert.isInner = false;
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
        if (prevFSegmentIt != outArgs->featherPolygon.end()) {
            ++prevFSegmentIt;
        }
        if (nextFSegmentIt != outArgs->featherPolygon.end()) {
            ++nextFSegmentIt;
        }
    }



    // Now tesselate the internal bezier using glu
    libtess_GLUtesselator* tesselator = libtess_gluNewTess();

    libtess_gluTessCallback(tesselator, LIBTESS_GLU_TESS_BEGIN_DATA, (void (*)())tess_begin_primitive_callback);
    libtess_gluTessCallback(tesselator, LIBTESS_GLU_TESS_VERTEX_DATA, (void (*)())tess_vertex_callback);
    libtess_gluTessCallback(tesselator, LIBTESS_GLU_TESS_END_DATA, (void (*)())tess_end_primitive_callback);
    libtess_gluTessCallback(tesselator, LIBTESS_GLU_TESS_ERROR_DATA, (void (*)())tess_error_callback);
    libtess_gluTessCallback(tesselator, LIBTESS_GLU_TESS_COMBINE_DATA, (void (*)())tess_intersection_combine_callback);

    // From README: if you know that all polygons lie in the x-y plane, call
    // gluTessNormal(tess, 0.0, 0.0, 1.0) before rendering any polygons.
    libtess_gluTessNormal(tesselator, 0, 0, 1);
    libtess_gluTessBeginPolygon(tesselator, (void*)outArgs);
    libtess_gluTessBeginContour(tesselator);

    // Join the internal polygon into a single vector of vertices now that we don't need per-bezier segments separation.
    // we will need the indices for libtess
    for (std::vector<std::vector<ParametricPoint> >::const_iterator it = outArgs->bezierPolygon.begin(); it != outArgs->bezierPolygon.end(); ++it) {

        // don't add the first vertex which is the same as the last vertex of the last segment
        std::vector<ParametricPoint>::const_iterator start = it->begin();
        ++start;
        outArgs->bezierPolygonJoined.insert(outArgs->bezierPolygonJoined.end(), start, it->end());


    }
    outArgs->bezierPolygon.clear();

    outArgs->bezierPolygonIndices.resize(outArgs->bezierPolygonJoined.size());

    for (std::size_t i = 0; i < outArgs->bezierPolygonIndices.size(); ++i) {
        double coords[3] = {outArgs->bezierPolygonJoined[i].x, outArgs->bezierPolygonJoined[i].y, 1.};
        unsigned int* vertexData =  new unsigned int;
        *vertexData = i;
        outArgs->bezierPolygonIndices[i] = vertexData;
        assert(*vertexData >= 0 && *vertexData < outArgs->bezierPolygonJoined.size());
        libtess_gluTessVertex(tesselator, coords, (void*)(vertexData) /*per-vertex client data*/);
    }

    libtess_gluTessEndContour(tesselator);
    libtess_gluTessEndPolygon(tesselator);
    libtess_gluDeleteTess(tesselator);
    
    // now delete indices
    for (std::size_t i = 0; i < outArgs->bezierPolygonIndices.size(); ++i) {
        delete outArgs->bezierPolygonIndices[i];
    }
    outArgs->bezierPolygonIndices.clear();
    
    // check for errors
    assert(outArgs->error == 0);
    
} // RotoBezierTriangulation::computeTriangles

NATRON_NAMESPACE_EXIT;
