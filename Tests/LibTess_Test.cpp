/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2023 The Natron developers
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

#include "Global/Macros.h"

#include <cmath>
#include <limits>
#include <list>
#include <vector>

#include <gtest/gtest.h>

#include "libs/libtess/libtess.h"

// Tests of the bundled GLU tessellator (libs/libtess), used by Roto/RotoPaint
// (RotoContextPrivate::computeTriangles()) to fill the shapes. They call it the
// way Natron does: one contour, normal (0,0,1), odd winding rule, and a combine
// callback. The result is checked against the odd winding rule at sample points.

namespace {

struct TessPoint
{
    double x, y;
};

struct TessTriangle
{
    TessPoint p[3];
};

struct TessResult
{
    std::vector<TessTriangle> triangles;
    std::vector<unsigned int> errors;
    int protocolErrors; // vertex outside begin/end, nested begin, short primitive...

    TessResult()
        : triangles()
        , errors()
        , protocolErrors(0)
    {
    }
};

struct TessState
{
    TessResult* result;
    unsigned int primitive;
    bool inPrimitive;
    std::vector<const TessPoint*> vertices;
    std::list<TessPoint> combined; // stable addresses
};

void
tessBegin(unsigned int which,
          void* polygonData)
{
    TessState* s = static_cast<TessState*>(polygonData);

    if (s->inPrimitive) {
        ++s->result->protocolErrors;
    }
    s->inPrimitive = true;
    s->primitive = which;
    s->vertices.clear();
}

void
tessVertex(void* data,
           void* polygonData)
{
    TessState* s = static_cast<TessState*>(polygonData);

    if (!s->inPrimitive || !data) {
        ++s->result->protocolErrors;

        return;
    }
    s->vertices.push_back( static_cast<const TessPoint*>(data) );
}

void
tessEnd(void* polygonData)
{
    TessState* s = static_cast<TessState*>(polygonData);

    if (!s->inPrimitive || s->vertices.size() < 3) {
        ++s->result->protocolErrors;
    }
    s->inPrimitive = false;
    const std::vector<const TessPoint*>& v = s->vertices;
    std::vector<TessTriangle>& tris = s->result->triangles;
    switch (s->primitive) {
    case LIBTESS_GL_TRIANGLES:
        if (v.size() % 3 != 0) {
            ++s->result->protocolErrors;
        }
        for (std::size_t i = 0; i + 2 < v.size(); i += 3) {
            TessTriangle t = { { *v[i], *v[i + 1], *v[i + 2] } };
            tris.push_back(t);
        }
        break;
    case LIBTESS_GL_TRIANGLE_FAN:
        for (std::size_t i = 2; i < v.size(); ++i) {
            TessTriangle t = { { *v[0], *v[i - 1], *v[i] } };
            tris.push_back(t);
        }
        break;
    case LIBTESS_GL_TRIANGLE_STRIP:
        for (std::size_t i = 2; i < v.size(); ++i) {
            TessTriangle t = { { *v[i - 2], *v[i - 1], *v[i] } };
            tris.push_back(t);
        }
        break;
    default:
        ++s->result->protocolErrors;
        break;
    }
}

void
tessError(unsigned int error,
          void* polygonData)
{
    TessState* s = static_cast<TessState*>(polygonData);

    s->result->errors.push_back(error);
}

void
tessCombine(double coords[3],
            void* /*data*/[4],
            double /*w*/[4],
            void** dataOut,
            void* polygonData)
{
    TessState* s = static_cast<TessState*>(polygonData);
    TessPoint p = { coords[0], coords[1] };

    s->combined.push_back(p);
    *dataOut = &s->combined.back();
}

// Tessellate one contour like RotoContextPrivate::computeTriangles() does.
TessResult
tessellate(const std::vector<TessPoint>& contour)
{
    TessResult result;
    TessState state;

    state.result = &result;
    state.primitive = 0;
    state.inPrimitive = false;

    libtess_GLUtesselator* tess = libtess_gluNewTess();
    if (!tess) {
        ADD_FAILURE() << "libtess_gluNewTess() failed";

        return result;
    }
    libtess_gluTessCallback(tess, LIBTESS_GLU_TESS_BEGIN_DATA, (void (*)())tessBegin);
    libtess_gluTessCallback(tess, LIBTESS_GLU_TESS_VERTEX_DATA, (void (*)())tessVertex);
    libtess_gluTessCallback(tess, LIBTESS_GLU_TESS_END_DATA, (void (*)())tessEnd);
    libtess_gluTessCallback(tess, LIBTESS_GLU_TESS_ERROR_DATA, (void (*)())tessError);
    libtess_gluTessCallback(tess, LIBTESS_GLU_TESS_COMBINE_DATA, (void (*)())tessCombine);
    libtess_gluTessNormal(tess, 0, 0, 1);
    libtess_gluTessBeginPolygon(tess, &state);
    libtess_gluTessBeginContour(tess);
    for (std::size_t i = 0; i < contour.size(); ++i) {
        double coords[3] = { contour[i].x, contour[i].y, 1. };
        libtess_gluTessVertex( tess, coords, const_cast<TessPoint*>(&contour[i]) );
    }
    libtess_gluTessEndContour(tess);
    libtess_gluTessEndPolygon(tess);
    libtess_gluDeleteTess(tess);
    if (state.inPrimitive) {
        ++result.protocolErrors;
    }

    return result;
}

// Is (px,py) inside the contour, by the odd winding rule (crossing number)?
bool
insideOdd(const std::vector<TessPoint>& c,
          double px,
          double py)
{
    bool inside = false;

    for (std::size_t i = 0, j = c.size() - 1; i < c.size(); j = i++) {
        if ( ( (c[i].y > py) != (c[j].y > py) ) &&
             ( px < (c[j].x - c[i].x) * (py - c[i].y) / (c[j].y - c[i].y) + c[i].x ) ) {
            inside = !inside;
        }
    }

    return inside;
}

double
cross(const TessPoint& a,
      const TessPoint& b,
      double px,
      double py)
{
    return (b.x - a.x) * (py - a.y) - (b.y - a.y) * (px - a.x);
}

// Number of triangles which contain (px,py) strictly inside.
int
coverage(const TessResult& r,
         double px,
         double py)
{
    int n = 0;

    for (std::size_t i = 0; i < r.triangles.size(); ++i) {
        const TessPoint* p = r.triangles[i].p;
        double d0 = cross(p[0], p[1], px, py);
        double d1 = cross(p[1], p[2], px, py);
        double d2 = cross(p[2], p[0], px, py);
        if ( ( (d0 > 0) && (d1 > 0) && (d2 > 0) ) || ( (d0 < 0) && (d1 < 0) && (d2 < 0) ) ) {
            ++n;
        }
    }

    return n;
}

double
segmentDistance(const TessPoint& a,
                const TessPoint& b,
                double px,
                double py)
{
    double dx = b.x - a.x, dy = b.y - a.y;
    double l2 = dx * dx + dy * dy;
    double t = l2 > 0 ? ( (px - a.x) * dx + (py - a.y) * dy ) / l2 : 0.;

    t = t < 0 ? 0 : (t > 1 ? 1 : t);
    double qx = a.x + t * dx - px, qy = a.y + t * dy - py;

    return std::sqrt(qx * qx + qy * qy);
}

// Count the points of a regular grid over the bounding box of the contour where
// the tessellation doesn't cover the inside (by the odd winding rule) exactly
// once and the outside not at all. Points too close to an edge are skipped.
int
countCoverageErrors(const std::vector<TessPoint>& contour,
                    const TessResult& r,
                    int gridSize = 97)
{
    double xmin = std::numeric_limits<double>::infinity(), xmax = -xmin;
    double ymin = xmin, ymax = -xmin;

    for (std::size_t i = 0; i < contour.size(); ++i) {
        xmin = std::min(xmin, contour[i].x);
        xmax = std::max(xmax, contour[i].x);
        ymin = std::min(ymin, contour[i].y);
        ymax = std::max(ymax, contour[i].y);
    }
    double eps = 1e-6 * ( (xmax - xmin) + (ymax - ymin) );
    int errors = 0;
    for (int j = 0; j < gridSize; ++j) {
        for (int i = 0; i < gridSize; ++i) {
            double px = xmin + (xmax - xmin) * (i + 0.5) / gridSize;
            double py = ymin + (ymax - ymin) * (j + 0.5) / gridSize;
            bool nearEdge = false;
            for (std::size_t k = 0, l = contour.size() - 1; k < contour.size() && !nearEdge; l = k++) {
                nearEdge = segmentDistance(contour[l], contour[k], px, py) < eps;
            }
            if (nearEdge) {
                continue;
            }
            int expected = insideOdd(contour, px, py) ? 1 : 0;
            if (coverage(r, px, py) != expected) {
                ++errors;
            }
        }
    }

    return errors;
}

std::vector<TessPoint>
circle(int n,
       double radius)
{
    std::vector<TessPoint> c(n);

    for (int i = 0; i < n; ++i) {
        double a = 2 * M_PI * i / n;
        c[i].x = radius * std::cos(a);
        c[i].y = radius * std::sin(a);
    }

    return c;
}

bool
hasError(const TessResult& r,
         unsigned int error)
{
    for (std::size_t i = 0; i < r.errors.size(); ++i) {
        if (r.errors[i] == error) {
            return true;
        }
    }

    return false;
}
} // anon namespace

// A NaN coordinate (e.g. from a NaN control point) crashed the sweep (NULL
// dereference in ConnectLeftVertex()). It must be rejected with an error, and the
// other vertices tessellated. 200 vertices, to skip the single-fan fast path.
TEST(LibTess,
     NaNVertexIsSkipped)
{
    const double nan = std::numeric_limits<double>::quiet_NaN();
    const int n = 200;
    std::vector<TessPoint> ref = circle(n, 100.);

    for (int which = 0; which < 3; ++which) {
        std::vector<TessPoint> c = ref;
        TessPoint bad = c[n / 4];
        if (which == 0) {
            bad.x = nan;
        } else if (which == 1) {
            bad.y = nan;
        } else {
            bad.x = bad.y = nan;
        }
        c.insert(c.begin() + n / 4, bad);
        c.insert(c.begin() + 3 * n / 4, bad);

        TessResult r = tessellate(c);
        EXPECT_TRUE( hasError(r, LIBTESS_GLU_TESS_COORD_TOO_LARGE) ) << "case " << which;
        EXPECT_EQ(0, r.protocolErrors) << "case " << which;
        EXPECT_EQ(0, countCoverageErrors(ref, r) ) << "case " << which;
    }
}
