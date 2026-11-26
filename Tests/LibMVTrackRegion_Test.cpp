/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2026 The Natron developers
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


// libmv's region tracker (libmv::TrackRegion, which the Tracker node runs for each track and frame) on synthetic
// images, with each motion model: it refines the warp with the bundled Ceres Solver. Unlike the Tracker node
// tests, it needs no plug-in, so it runs on every CI build.

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <cmath>
#include <cstdio>
#include <string>

#include <gtest/gtest.h>

#include <libmv/image/image.h>
#include <libmv/tracking/track_region.h>

namespace {
// A 2D projective warp: (x, y) -> (h00 x + h01 y + h02, h10 x + h11 y + h12) / (h20 x + h21 y + 1)
struct Warp
{
    double h[3][3];
};

struct Point
{
    double x;
    double y;
};

Point
apply(const Warp& w,
      const Point& p)
{
    const double d = w.h[2][0] * p.x + w.h[2][1] * p.y + w.h[2][2];

    return Point{ (w.h[0][0] * p.x + w.h[0][1] * p.y + w.h[0][2]) / d,
                  (w.h[1][0] * p.x + w.h[1][1] * p.y + w.h[1][2]) / d };
}

// A smooth, non-periodic texture: a sum of Gaussian blobs
double
texture(const Point& p)
{
    static const double blobs[][4] = { // x, y, sigma, amplitude
        { 40., 45., 6., 1. }, { 70., 38., 9., -0.7 }, { 88., 70., 7., 0.9 }, { 55., 85., 10., 0.6 },
        { 64., 64., 5., -0.8 }, { 30., 75., 8., 0.5 }, { 95., 40., 6., 0.4 }, { 75., 95., 7., -0.5 }
    };
    double v = 0.2;

    for (const auto& b : blobs) {
        const double dx = p.x - b[0];
        const double dy = p.y - b[1];
        v += b[3] * std::exp( -(dx * dx + dy * dy) / (2. * b[2] * b[2]) );
    }

    return v;
}

const int kImageSize = 128;

// The image of the texture moved by the warp w: image(w(p)) = texture(p), so image(q) = texture(w^-1(q))
libmv::FloatImage
warpedImage(const Warp& inverse)
{
    libmv::FloatImage image(kImageSize, kImageSize, 1);

    for (int y = 0; y < kImageSize; ++y) {
        for (int x = 0; x < kImageSize; ++x) {
            image(y, x, 0) = (float)texture( apply( inverse, Point{ (double)x, (double)y } ) );
        }
    }

    return image;
}

// The inverse of a 3x3 matrix (the warps below are far from singular)
Warp
inverse(const Warp& w)
{
    const double (&m)[3][3] = w.h;
    const double det = m[0][0] * (m[1][1] * m[2][2] - m[1][2] * m[2][1]) -
                       m[0][1] * (m[1][0] * m[2][2] - m[1][2] * m[2][0]) +
                       m[0][2] * (m[1][0] * m[2][1] - m[1][1] * m[2][0]);
    Warp r;

    r.h[0][0] = (m[1][1] * m[2][2] - m[1][2] * m[2][1]) / det;
    r.h[0][1] = (m[0][2] * m[2][1] - m[0][1] * m[2][2]) / det;
    r.h[0][2] = (m[0][1] * m[1][2] - m[0][2] * m[1][1]) / det;
    r.h[1][0] = (m[1][2] * m[2][0] - m[1][0] * m[2][2]) / det;
    r.h[1][1] = (m[0][0] * m[2][2] - m[0][2] * m[2][0]) / det;
    r.h[1][2] = (m[0][2] * m[1][0] - m[0][0] * m[1][2]) / det;
    r.h[2][0] = (m[1][0] * m[2][1] - m[1][1] * m[2][0]) / det;
    r.h[2][1] = (m[0][1] * m[2][0] - m[0][0] * m[2][1]) / det;
    r.h[2][2] = (m[0][0] * m[1][1] - m[0][1] * m[1][0]) / det;

    return r;
}

// A rotation by a (radians) and a scale by s around (64, 64), then a translation by (tx, ty)
Warp
similarity(double tx,
           double ty,
           double a,
           double s)
{
    const double c = s * std::cos(a);
    const double d = s * std::sin(a);

    return Warp{ { { c, -d, 64. - c * 64. + d * 64. + tx },
                   { d, c, 64. - d * 64. - c * 64. + ty },
                   { 0., 0., 1. } } };
}

// Tracks the pattern (a 31x31 square around the center of the image, and its center) from the identity, with the
// motion model mode, between the texture and the texture moved by warp: it must be found where the warp takes it
void
expectRecovers(libmv::TrackRegionOptions::Mode mode,
               const Warp& warp)
{
    const Warp identity{ { { 1., 0., 0. }, { 0., 1., 0. }, { 0., 0., 1. } } };
    const libmv::FloatImage image1 = warpedImage(identity);
    const libmv::FloatImage image2 = warpedImage( inverse(warp) );
    // the four corners of the pattern, then its center (an extra point, as the Tracker node does)
    const double x1[5] = { 49., 79., 79., 49., 64. };
    const double y1[5] = { 49., 49., 79., 79., 64. };
    double x2[5], y2[5];

    for (int i = 0; i < 5; ++i) {
        x2[i] = x1[i];
        y2[i] = y1[i];
    }
    libmv::TrackRegionOptions options;
    options.mode = mode;
    options.num_extra_points = 1;
    options.use_brute_initialization = false;
    options.max_iterations = 50;
    libmv::TrackRegionResult result;
    libmv::TrackRegion(image1, image2, x1, y1, options, x2, y2, &result);
    ASSERT_TRUE( result.is_usable() ) << "termination " << (int)result.termination;
    for (int i = 0; i < 5; ++i) {
        const Point expected = apply( warp, Point{ x1[i], y1[i] } );
        EXPECT_NEAR(expected.x, x2[i], 0.05) << "point " << i;
        EXPECT_NEAR(expected.y, y2[i], 0.05) << "point " << i;
        // all the digits in the XML output (--gtest_output=xml), to compare two builds exactly
        char buf[64];
        std::snprintf(buf, sizeof(buf), "%.17g", x2[i]);
        ::testing::Test::RecordProperty("x" + std::to_string(i), buf);
        std::snprintf(buf, sizeof(buf), "%.17g", y2[i]);
        ::testing::Test::RecordProperty("y" + std::to_string(i), buf);
    }
}
} // anonymous namespace

TEST(LibMVTrackRegion, Translation)
{
    expectRecovers( libmv::TrackRegionOptions::TRANSLATION, similarity(2.3, -1.7, 0., 1.) );
}

TEST(LibMVTrackRegion, TranslationRotation)
{
    expectRecovers( libmv::TrackRegionOptions::TRANSLATION_ROTATION, similarity(2.3, -1.7, 0.04, 1.) );
}

TEST(LibMVTrackRegion, TranslationScale)
{
    expectRecovers( libmv::TrackRegionOptions::TRANSLATION_SCALE, similarity(2.3, -1.7, 0., 1.03) );
}

TEST(LibMVTrackRegion, TranslationRotationScale)
{
    expectRecovers( libmv::TrackRegionOptions::TRANSLATION_ROTATION_SCALE, similarity(2.3, -1.7, 0.04, 1.03) );
}

TEST(LibMVTrackRegion, Affine)
{
    expectRecovers( libmv::TrackRegionOptions::AFFINE, Warp{ { { 1.02, 0.03, -2.4 }, { -0.02, 0.98, 1.8 }, { 0., 0., 1. } } } );
}

TEST(LibMVTrackRegion, Homography)
{
    expectRecovers( libmv::TrackRegionOptions::HOMOGRAPHY,
                    Warp{ { { 1.02, 0.03, -2.4 }, { -0.02, 0.98, 1.8 }, { 0.0001, -0.00008, 1. } } } );
}
