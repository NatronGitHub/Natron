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

#include <algorithm>
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
            // offsets which keep the points off the lines through the vertices
            // (and so off the triangle edges) for the integer coordinates below
            double px = xmin + (xmax - xmin) * (i + 0.381966) / gridSize;
            double py = ymin + (ymax - ymin) * (j + 0.618034) / gridSize;
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

// Contours of fewer than 100 vertices are first tried as a single triangle fan
// from their first vertex (__gl_renderCache()). That fan was used whenever its
// triangles had consistent orientations, even when they overlapped: for a
// contour winding twice around its first vertex, or passing through it again,
// an area with winding number 2 was filled although the odd rule makes it a hole.
TEST(LibTess,
     FanFastPathOverlap)
{
    // winds twice around the first vertex: (0.2,0.2) has winding number 2
    const TessPoint spiral[] = {
        { 0, 0 }, { 1, 0 }, { 0, 1 }, { -1, 0 }, { 0, -1 }, { 2, 0.1 }, { 0, 2 }
    };
    // passes through its first vertex again
    const TessPoint touching[] = {
        { 0, 3 }, { 4, 3 }, { 2, 2 }, { 0, 2 }, { 0, 2 }, { 0, 3 }, { 2, 3 }, { 4, 1 }, { 2, 0 }
    };
    const struct
    {
        const TessPoint* p;
        std::size_t n;
    } cases[] = {
        { spiral, sizeof(spiral) / sizeof(spiral[0]) },
        { touching, sizeof(touching) / sizeof(touching[0]) },
    };

    for (std::size_t k = 0; k < sizeof(cases) / sizeof(cases[0]); ++k) {
        std::vector<TessPoint> c(cases[k].p, cases[k].p + cases[k].n);
        // both orientations, and the vertices mirrored
        for (int variant = 0; variant < 4; ++variant) {
            std::vector<TessPoint> v = c;
            if (variant & 1) {
                std::reverse( v.begin() + 1, v.end() );
            }
            if (variant & 2) {
                for (std::size_t i = 0; i < v.size(); ++i) {
                    v[i].y = -v[i].y;
                }
            }
            TessResult r = tessellate(v);
            EXPECT_TRUE( r.errors.empty() ) << "case " << k << " variant " << variant;
            EXPECT_EQ(0, r.protocolErrors) << "case " << k << " variant " << variant;
            EXPECT_EQ(0, countCoverageErrors(v, r) ) << "case " << k << " variant " << variant;
        }
    }

    std::vector<TessPoint> c( spiral, spiral + sizeof(spiral) / sizeof(spiral[0]) );
    TessResult r = tessellate(c);
    EXPECT_FALSE( insideOdd(c, 0.2, 0.2) );
    EXPECT_EQ( 0, coverage(r, 0.2, 0.2) );
}

// The fast path still renders simple contours, including an explicitly closed one
// (last vertex == first vertex).
TEST(LibTess,
     FanFastPathSimple)
{
    std::vector<TessPoint> c = circle(64, 10.);

    c.push_back(c[0]);
    TessResult r = tessellate(c);
    EXPECT_TRUE( r.errors.empty() );
    EXPECT_EQ(0, r.protocolErrors);
    EXPECT_EQ(0, countCoverageErrors(c, r) );
}

// Nearly parallel overlapping edges: the intersection of two edges on the
// diagonal x == y was computed at (4, 3.24), far from both, and the edges were
// moved there, which filled wrong areas.
TEST(LibTess,
     OverlappingCollinearEdges)
{
    const TessPoint diagonal[] = {
        { 4, 4 }, { 0, 0 }, { 5, 5 }, { 1, 1 }, { 0, 5 }, { 0, 0 }, { 2, 5 }, { 4, 0 }, { 0, 2 }, { 3, 0 }
    };
    const struct
    {
        const TessPoint* p;
        std::size_t n;
    } cases[] = {
        { diagonal, sizeof(diagonal) / sizeof(diagonal[0]) },
    };

    for (std::size_t k = 0; k < sizeof(cases) / sizeof(cases[0]); ++k) {
        std::vector<TessPoint> c(cases[k].p, cases[k].p + cases[k].n);
        // both orientations, and the vertices mirrored
        for (int variant = 0; variant < 4; ++variant) {
            std::vector<TessPoint> v = c;
            if (variant & 1) {
                std::reverse( v.begin(), v.end() );
            }
            if (variant & 2) {
                for (std::size_t i = 0; i < v.size(); ++i) {
                    v[i].y = -v[i].y;
                }
            }
            TessResult r = tessellate(v);
            EXPECT_TRUE( r.errors.empty() ) << "case " << k << " variant " << variant;
            EXPECT_EQ(0, r.protocolErrors) << "case " << k << " variant " << variant;
            EXPECT_EQ(0, countCoverageErrors(v, r) ) << "case " << k << " variant " << variant;
        }
    }
}

// Contours collapsed onto a line (zero area), found by fuzzing on macOS arm64,
// where they made the sweep use an edge after it was freed
// (heap-use-after-free, found with AddressSanitizer) or output a face which
// reaches the sweep sentinels, with NULL vertex data (the Roto callbacks then
// crashed). The sweep's predicates (EdgeSign(), EdgeEval()) were then
// compiled with floating-point contraction (fused multiply-add, clang's
// default on arm64): for points exactly on x == y, EdgeSign() computes
// a*b - b*a, which is exactly 0 without FMA but the rounding error of a*b
// with it, so collinear points were seen on random sides of each other, and
// the sweep reached these inconsistent states. libtess is now compiled with
// -ffp-contract=off, so these contours are tessellated (to nothing) the same
// way on every platform; with FMA they were dropped (#1510).
// - selfLoop: overlapping edges and repeated points on a small integer grid.
// - diagonal, diagonal2: collinear points on the diagonal x == y with rounding
//   noise (a Roto shape collapsed onto a 45 degree line).
// - collapsed, collapsed2: a Roto shape collapsed onto the diagonal x == y,
//   with repeated points.
// - sliver: a 45 degree sliver (width 1e-7 of its length).
// - sentinel: a contour folding back along the diagonal.
TEST(LibTess,
     CollapsedContoursDontDependOnFma)
{
    const TessPoint selfLoop[] = {
        { 4, 4 }, { 1, 1 }, { 1, 5 }, { 2, 2 }, { 3, 3 }, { 4, 5 }, { 0, 0 }, { 2, 3 }, { 0, 4 }, { 3, 0 },
        { 0, 3 }, { 5, 4 }, { 0, 4 }, { 1, 0 }, { 4, 5 }, { 0, 3 }, { 5, 1 }, { 1, 5 }, { 0, 0 }, { 4, 0 },
        { 1, 2 }, { 5, 3 }, { 1, 0 }, { 0, 3 }, { 5, 2 }, { 5, 5 }, { 0, 3 }, { 3, 5 }, { 5, 0 }, { 0, 4 },
        { 3, 0 }, { 1, 3 }, { 3, 2 }, { 0, 2 }, { 2, 5 }, { 4, 1 }, { 1, 2 }, { 3, 1 }, { 0, 1 }, { 0, 3 },
        { 4, 1 }
    };
    const TessPoint diagonal[] = {
        { 2.8593724757887253, 2.8593724757887253 }, { 0.3311291254383543, 0.3311291254383543 },
        { 0.41163672074104585, 0.41163672074104585 }, { 0.38654025246153023, 0.38654025246153023 },
        { -0.1699246436876148, -0.1699246436876148 }, { 7.74596319687721, 7.74596319687721 }
    };
    const TessPoint diagonal2[] = {
        { 1024.2185654768973, 1024.2185654768973 }, { 949.6820899775199, 949.6820899775199 },
        { 949.454134087798, 949.454134087798 }, { 949.2291316844357, 949.2291316844357 },
        { 950.2348319234983, 950.2348319234983 }, { 949.5469776518655, 949.5469776518655 },
        { 949.5469776518659, 949.5469776518659 }, { 949.5469776518657, 949.5469776518657 },
        { 948.5883779708273, 948.5883779708273 }
    };
    const TessPoint collapsed[] = {
        { 1492.703538304817, 1444.0105249271114 }, { 1484.5355915698049, 1435.8425781920996 },
        { 1476.3676448347928, 1427.6746314570878 }, { 1468.199698099781, 1419.5066847220758 },
        { 1460.0317513647692, 1411.338737987064 }, { 1451.8638046297572, 1403.170791252052 },
        { 1443.6958578947454, 1395.00284451704 }, { 1443.6958578947454, 1395.00284451704 },
        { 1451.4333301727079, 1402.7403167950026 }, { 1459.1708024506704, 1410.4777890729652 },
        { 1466.908274728633, 1418.2152613509277 }, { 1474.6457470065957, 1425.9527336288902 },
        { 1482.383219284558, 1433.690205906853 }, { 1490.1206915625207, 1441.4276781848155 },
        { 1448.456086088662, 1399.7630727109567 }, { 1448.456086088662, 1399.7630727109567 },
        { 1406.7914806148035, 1358.0984672370982 }, { 1365.1268751409448, 1316.4338617632395 },
        { 1323.4622696670863, 1274.769256289381 }, { 1281.7976641932275, 1233.1046508155223 },
        { 1281.7976641932275, 1233.1046508155223 }, { 1240.133058719369, 1191.4400453416638 },
        { 1282.228138650277, 1233.5351252725718 }, { 1324.3232185811848, 1275.6302052034796 },
        { 1324.3232185811848, 1275.6302052034796 }, { 1366.4182985120929, 1317.7252851343876 },
        { 1408.5133784430009, 1359.8203650652956 }, { 1450.608458373909, 1401.9154449962036 },
        { 1450.608458373909, 1401.9154449962036 }
    };
    const TessPoint collapsed2[] = {
        { 1303.2441610714764, 1896.9126562871368 }, { 1303.2441610714764, 1896.9126562871368 },
        { 1304.4932712019654, 1898.1617664176258 }, { 1305.7423813324544, 1899.4108765481149 },
        { 1306.9914914629435, 1900.659986678604 }, { 1308.2406015934325, 1901.909096809093 },
        { 1309.4897117239216, 1903.158206939582 }, { 1309.4897117239216, 1903.158206939582 },
        { 1310.7388218544108, 1904.4073170700713 }, { 1311.9879319849, 1905.6564272005603 },
        { 1311.0050409471014, 1904.6735361627618 }, { 1310.0221499093032, 1903.6906451249636 },
        { 1309.039258871505, 1902.7077540871653 }, { 1308.0563678337066, 1901.724863049367 },
        { 1307.0734767959082, 1900.7419720115686 }, { 1306.09058575811, 1899.7590809737703 },
        { 1305.1076947203117, 1898.776189935972 }, { 1305.1606074492186, 1898.829102664879 },
        { 1305.2135201781257, 1898.882015393786 }, { 1305.2664329070326, 1898.934928122693 },
        { 1305.3193456359397, 1898.9878408516001 }, { 1305.3722583648466, 1899.040753580507 },
        { 1305.4251710937538, 1899.0936663094142 }, { 1305.4780838226607, 1899.146579038321 },
        { 1306.6763685703058, 1900.3448637859663 }, { 1307.874653317951, 1901.5431485336114 },
        { 1309.0729380655962, 1902.7414332812566 }, { 1310.2712228132414, 1903.9397180289018 },
        { 1311.4695075608865, 1905.138002776547 }, { 1312.6677923085317, 1906.3362875241921 },
        { 1313.866077056177, 1907.5345722718373 }, { 1313.3602805381443, 1907.0287757538047 },
        { 1312.8544840201114, 1906.5229792357718 }, { 1312.3486875020787, 1906.0171827177392 },
        { 1311.8428909840459, 1905.5113861997063 }, { 1311.3370944660132, 1905.0055896816737 },
        { 1310.8312979479804, 1904.4997931636408 }, { 1310.3255014299477, 1903.9939966456081 },
        { 1309.247035905492, 1902.9155311211525 }, { 1308.1685703810363, 1901.8370655966967 },
        { 1307.0901048565806, 1900.758600072241 }, { 1306.0116393321248, 1899.6801345477852 },
        { 1306.0116393321248, 1899.6801345477852 }, { 1304.9331738076692, 1898.6016690233296 },
        { 1303.8547082832133, 1897.5232034988737 }, { 1302.7762427587577, 1896.444737974418 },
        { 1303.9389506733153, 1897.6074458889757 }, { 1305.1016585878729, 1898.7701538035333 },
        { 1306.2643665024304, 1899.9328617180909 }, { 1307.427074416988, 1901.0955696326484 },
        { 1308.5897823315456, 1902.258277547206 }, { 1309.7524902461032, 1903.4209854617636 },
        { 1310.915198160661, 1904.5836933763214 }, { 1309.8193357193488, 1903.4878309350092 },
        { 1308.7234732780366, 1902.391968493697 }, { 1307.6276108367247, 1901.296106052385 },
        { 1306.5317483954125, 1900.200243611073 }, { 1305.4358859541005, 1899.104381169761 },
        { 1304.3400235127883, 1898.0085187284487 }
    };
    const TessPoint sliver[] = {
        { 1409.7676432692938, 1228.0464398952952 }, { 1478.2417793589657, 1296.520575984967 },
        { 1274.1021644046093, 1092.380966917197 }, { 1264.338881320849, 1082.6176779468503 },
        { 1565.94123675494, 1384.2200333809415 }, { 1447.552147703637, 1265.8309992752952 },
        { 1279.3771647780907, 1097.6559687826739 }, { 1279.3771647780907, 1097.6559687826739 },
        { 1272.3343530274046, 1090.613149653406 }, { 1501.416599422356, 1319.6953960483575 },
        { 1518.5978348052506, 1336.8767064716667 }, { 1542.5314313089618, 1360.810309744822 },
        { 1475.7581966720275, 1294.0370562215621 }, { 1493.4657958459575, 1311.7446604039585 },
        { 1257.5428642572315, 1075.821662086141 }, { 1376.581873192369, 1194.8607046905995 },
        { 1603.7848620909863, 1422.0637578519356 }, { 1423.1535072972094, 1241.4323519678892 },
        { 1528.2583352786464, 1346.5371319046478 }, { 1382.3682804230443, 1200.6470770490457 },
        { 1266.8735840996346, 1085.1523845676704 }, { 1368.4396957326867, 1186.718492358688 },
        { 1530.9672742997902, 1349.2460709257916 }, { 1530.9672742997902, 1349.2460709257916 },
        { 1291.3805478801603, 1109.6593445061617 }, { 1291.3805478801603, 1109.6593445061617 },
        { 1553.1351332334918, 1371.4139298594932 }, { 1489.7807500774268, 1308.0596135931391 },
        { 1338.9617021821145, 1157.2405230397524 }, { 1545.0047305417745, 1363.2836096771894 },
        { 1310.1704615317433, 1128.4492581577447 }, { 1310.1704615317433, 1128.4492581577447 },
        { 1376.115528125934, 1194.3943247519353 }, { 1275.169903115779, 1093.4486997417805 },
        { 1275.4607884704428, 1093.7395850964442 }, { 1476.608152066502, 1294.8869486925034 },
        { 1282.3740011716009, 1100.6528060238177 }, { 1289.2726131129975, 1107.551409738999 },
        { 1289.2726131129975, 1107.551409738999 }, { 1269.532300055033, 1087.8110966810343 },
        { 1582.8865512596935, 1401.165347885695 }, { 1582.8865512596935, 1401.165347885695 },
        { 1376.8137393485122, 1195.0925709123242 }, { 1606.382692087259, 1424.6615885829856 },
        { 1429.5651738617162, 1247.8440203458892 }
    };
    const TessPoint sentinel[] = {
        { 0.0, 0.0 }, { 2.9912378823166774, 2.9912378823166774 }, { 0.325072093554872, 0.325072093554872 },
        { 0.25253005332581946, 0.25253005332581946 }, { -0.08464818798407475, -0.08464818798407475 },
        { 9.431137480475272, 9.431137480475272 }, { 9.260410117151062, 9.260410117151062 },
        { 11.83533486484881, 11.83533486484881 }, { 14.923815445781248, 14.923815445781248 },
        { 6.246650031036572, 2.5294825052934717 }, { 4.726345628711513, 4.726345628711513 },
        { 2.815603780993113, 2.815603780993113 }, { 1.1433086976458517, 1.1433086976458517 },
        { 9.364772280504637, 9.364772280504637 }
    };
    const struct
    {
        const char* name;
        const TessPoint* p;
        std::size_t n;
    } cases[] = {
        { "selfLoop", selfLoop, sizeof(selfLoop) / sizeof(selfLoop[0]) },
        { "diagonal", diagonal, sizeof(diagonal) / sizeof(diagonal[0]) },
        { "diagonal2", diagonal2, sizeof(diagonal2) / sizeof(diagonal2[0]) },
        { "collapsed", collapsed, sizeof(collapsed) / sizeof(collapsed[0]) },
        { "collapsed2", collapsed2, sizeof(collapsed2) / sizeof(collapsed2[0]) },
        { "sliver", sliver, sizeof(sliver) / sizeof(sliver[0]) },
        { "sentinel", sentinel, sizeof(sentinel) / sizeof(sentinel[0]) },
    };

    for (std::size_t k = 0; k < sizeof(cases) / sizeof(cases[0]); ++k) {
        std::vector<TessPoint> c(cases[k].p, cases[k].p + cases[k].n);
        TessResult r = tessellate(c);
        EXPECT_TRUE( r.errors.empty() ) << cases[k].name;
        EXPECT_EQ(0, r.protocolErrors) << cases[k].name; // includes NULL vertex data
        EXPECT_EQ(0, countCoverageErrors(c, r) ) << cases[k].name;
    }
}

// Degenerate contours which libtess can't tessellate, even without
// floating-point contraction: contours going back and forth along a line of
// slope about -0.51 or -1.85 (zero area), with repeated points, found by
// fuzzing. The polygon is now dropped with GLU_INVALID_OPERATION, and the
// tessellator stays usable. Before (with or without FMA):
// - lineEdgeInUse: WalkDirtyRegions() deleted an edge whose other half was
//   still the upper edge of an active region, and then used it
//   (heap-use-after-free). Now caught by DeleteRegionAndEdge().
// - lineSentinel: a face reaching the sentinels was marked inside, and was
//   output with NULL vertex data. Now caught by CheckInsideFaces().
// Only with NDEBUG: with assertions, an assertion of the sweep fails before
// (e.g. in AddRightEdges()), as it did before these fixes. So the Debug builds
// of the per-PR CI don't run this test; the release builds do.
#ifdef NDEBUG
TEST(LibTess,
     DegenerateContoursAreDropped)
{
    const TessPoint lineEdgeInUse[] = {
        { 18.836683691209664, -5.9765320931001797 }, { 23.180586171366546, -8.1856996038669827 },
        { 4.976016100439832, 1.07255260871873 }, { -3.5463539801287016, 5.4067528313819038 },
        { 0.8183567314968041, 3.1870029309028571 }, { 0.8183567314968041, 3.1870029309028571 },
        { -3.3778888042174273, 5.3210769224454397 }
    };
    const TessPoint lineSentinel[] = {
        { 46.35967163838238, -71.883248623010033 }, { 46.35967163838238, -71.883248623010033 },
        { 30.543305439446645, -42.718434516198542 }, { 22.057052120693598, -27.070086634534121 },
        { 8.8207140909198607, -2.662752279867588 }, { 8.8207140909198607, -2.662752279867588 },
        { -1.3793802172692189, 16.145857145873826 }, { -1.6842379506345568, 16.708003912760283 },
        { 14.109446117103076, -12.414985154619371 }, { 14.109446117103076, -12.414985154619371 },
        { -2.7244957733982815, 18.626202166547678 }, { -6.6184228103812419, 25.806464481587973 },
        { -26.400904180447618, 62.284653243319475 }, { -17.79616292042174, 46.417817831861164 },
        { -13.858951694821876, 39.157741019679229 }, { -13.858951694821876, 39.157741019679229 },
        { -8.5212664141548622, 29.315240092899195 }, { -11.116833950672413, 34.101373883181743 },
        { -13.23372368053775, 38.004842902525468 }, { -9.4061626454759448, 30.946957123580617 },
        { 4.0465616610603554, 6.1406142066648242 }, { 4.0465616610603554, 6.1406142066648242 },
        { -4.2828976296451682, 21.499839503308934 }, { -4.2828976296451682, 21.499839503308934 },
        { 3.8443677745609808, 6.5134525084034882 }, { 3.8443677745609808, 6.5134525084034882 },
        { -13.898420623970489, 39.230520314691489 }, { -21.126221547402427, 52.558326947489178 },
        { -21.126221547402427, 52.558326947489178 }, { -3.1179352380162548, 19.351690480585244 },
        { -11.773728042555021, 35.312663115907533 }, { -18.118590408885506, 47.012362599331496 },
        { -18.118590408885506, 47.012362599331496 }, { -30.79352530548562, 70.384489636966379 },
        { -23.651993991924201, 57.215761082109886 }, { -7.0656567219895479, 26.631147833646175 },
        { -7.0656567219895479, 26.631147833646175 }
    };
    const struct
    {
        const char* name;
        const TessPoint* p;
        std::size_t n;
    } cases[] = {
        { "lineEdgeInUse", lineEdgeInUse, sizeof(lineEdgeInUse) / sizeof(lineEdgeInUse[0]) },
        { "lineSentinel", lineSentinel, sizeof(lineSentinel) / sizeof(lineSentinel[0]) },
    };
    for (std::size_t k = 0; k < sizeof(cases) / sizeof(cases[0]); ++k) {
        std::vector<TessPoint> c(cases[k].p, cases[k].p + cases[k].n);
        TessResult r = tessellate(c);
        EXPECT_TRUE( hasError(r, LIBTESS_GLU_INVALID_OPERATION) ) << cases[k].name;
        EXPECT_EQ(0, r.protocolErrors) << cases[k].name; // includes NULL vertex data
    }

    // the tessellator is still usable
    std::vector<TessPoint> square = circle(4, 1.);
    TessResult r2 = tessellate(square);
    EXPECT_TRUE( r2.errors.empty() );
    EXPECT_EQ(0, countCoverageErrors(square, r2) );
}
#endif // NDEBUG

// A self-intersecting contour (117 points on a 10x10 integer grid, found by
// fuzzing, 1 contour in 8 million) on which the sweep never ended: two edges
// whose right endpoints are 1 ulp apart "intersect" 1 ulp to the left of both,
// CheckForIntersect() split them there, and WalkDirtyRegions() found the same
// configuration again 1 ulp further left, allocating memory very fast
// (gigabytes per second) for billions of splits. The splits are now limited
// (CountSplits()), and the polygon is dropped with GLU_INVALID_OPERATION,
// like the other degenerate cases (#1580). Without the limit, this test runs
// out of memory. With floating-point contraction (FMA), which libtess is
// compiled without (#1510), this contour happens to be tessellated. The test
// runs in every configuration: no sweep assertion fails on it.
TEST(LibTess,
     EndlessIntersectionsAreDropped)
{
    const TessPoint endless[] = {
        { 4, 1 }, { 8, 6 }, { 0, 3 }, { 3, 2 }, { 6, 4 }, { 6, 5 }, { 4, 1 }, { 1, 3 }, { 2, 1 }, { 1, 6 },
        { 1, 3 }, { 1, 2 }, { 1, 2 }, { 7, 6 }, { 1, 4 }, { 0, 2 }, { 1, 4 }, { 9, 8 }, { 7, 9 }, { 7, 4 },
        { 7, 4 }, { 2, 0 }, { 9, 1 }, { 5, 3 }, { 0, 1 }, { 1, 7 }, { 3, 0 }, { 2, 5 }, { 7, 7 }, { 7, 3 },
        { 2, 1 }, { 9, 7 }, { 3, 4 }, { 3, 6 }, { 6, 5 }, { 4, 4 }, { 6, 2 }, { 2, 9 }, { 5, 1 }, { 4, 4 },
        { 9, 5 }, { 2, 7 }, { 3, 6 }, { 9, 6 }, { 1, 6 }, { 1, 7 }, { 5, 8 }, { 4, 2 }, { 7, 2 }, { 6, 2 },
        { 5, 2 }, { 1, 5 }, { 0, 4 }, { 9, 5 }, { 1, 4 }, { 5, 6 }, { 2, 2 }, { 9, 6 }, { 9, 0 }, { 3, 0 },
        { 5, 1 }, { 9, 9 }, { 4, 0 }, { 8, 2 }, { 8, 2 }, { 6, 5 }, { 2, 7 }, { 9, 8 }, { 3, 9 }, { 6, 9 },
        { 0, 6 }, { 2, 0 }, { 2, 7 }, { 7, 2 }, { 2, 4 }, { 7, 8 }, { 2, 1 }, { 0, 6 }, { 4, 2 }, { 1, 7 },
        { 1, 2 }, { 8, 1 }, { 5, 2 }, { 2, 4 }, { 2, 2 }, { 3, 6 }, { 1, 1 }, { 6, 7 }, { 9, 8 }, { 2, 9 },
        { 1, 6 }, { 1, 7 }, { 6, 0 }, { 1, 8 }, { 6, 9 }, { 7, 6 }, { 1, 8 }, { 6, 4 }, { 3, 6 }, { 5, 0 },
        { 6, 8 }, { 9, 1 }, { 8, 0 }, { 4, 0 }, { 2, 1 }, { 3, 6 }, { 1, 3 }, { 4, 9 }, { 2, 4 }, { 9, 0 },
        { 8, 7 }, { 4, 9 }, { 5, 8 }, { 9, 1 }, { 7, 4 }, { 2, 8 }, { 8, 6 }
    };
    std::vector<TessPoint> c( endless, endless + sizeof(endless) / sizeof(endless[0]) );
    TessResult r = tessellate(c);
    EXPECT_TRUE( hasError(r, LIBTESS_GLU_INVALID_OPERATION) );
    EXPECT_TRUE( r.triangles.empty() );
    EXPECT_EQ(0, r.protocolErrors);

    // the tessellator is still usable
    std::vector<TessPoint> square = circle(4, 1.);
    TessResult r2 = tessellate(square);
    EXPECT_TRUE( r2.errors.empty() );
    EXPECT_EQ(0, countCoverageErrors(square, r2) );
}
