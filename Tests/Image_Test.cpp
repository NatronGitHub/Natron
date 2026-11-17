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

#include <cstring>
#include <vector>
#include <gtest/gtest.h>

#include "Engine/Image.h"
#include "Engine/ViewIdx.h"

NATRON_NAMESPACE_USING

TEST(BitmapTest,
     SimpleRect)
{
    RectI rod(0, 0, 100, 100);
    Bitmap bm(rod);

    ///assert that the union of all the non rendered rects is the rod
    std::list<RectI> nonRenderedRects;

    bm.minimalNonMarkedRects(rod, nonRenderedRects);
    RectI nonRenderedRectsUnion;

    for (std::list<RectI>::iterator it = nonRenderedRects.begin(); it != nonRenderedRects.end(); ++it) {
        nonRenderedRectsUnion.merge(*it);
    }

    ASSERT_TRUE(rod == nonRenderedRectsUnion);

    ///assert that the "underlying" bitmap is clean
    const char* map = bm.getBitmap();
    ASSERT_TRUE( !std::memchr( map, 1, rod.area() ) );
    ASSERT_TRUE( bm.isNonMarked(rod) );

    RectI halfRoD(0, 0, 100, 50);
    bm.markForRendered(halfRoD);

    ///assert that non of the rendered rects interesect the non rendered half
    RectI nonRenderedHalf(0, 50, 100, 100);
    nonRenderedRects.clear();
    bm.minimalNonMarkedRects(rod, nonRenderedRects);
    for (std::list<RectI>::iterator it = nonRenderedRects.begin(); it != nonRenderedRects.end(); ++it) {
        ASSERT_TRUE( (*it).intersects(nonRenderedHalf) );
    }


    ///assert that the underlying bitmap is marked as expected
    const char* start = map;

    ///check that there are only ones in the rendered half
    ASSERT_TRUE( !memchr( start, 0, halfRoD.area() ) );

    ///check that there are only 0s in the non rendered half
    start = map + halfRoD.area();
    ASSERT_TRUE( !memchr( start, 1, halfRoD.area() ) );

    ///mark for renderer the other half of the rod
    bm.markForRendered(nonRenderedHalf);

    ///assert that the bm is rendered totally
    nonRenderedRects.clear();
    bm.minimalNonMarkedRects(rod, nonRenderedRects);
    ASSERT_TRUE( nonRenderedRects.empty() );
    ASSERT_TRUE( !memchr( map, 0, rod.area() ) );

    ///More complex example where A,B,C,D are not rendered check that both trimap & bitmap yield the same result
    // BBBBBBBBBBBBBB
    // BBBBBBBBBBBBBB
    // CXXXXXXXXXXDDD
    // CXXXXXXXXXXDDD
    // CXXXXXXXXXXDDD
    // CXXXXXXXXXXDDD
    // AAAAAAAAAAAAAA
    bm.clear(rod);

    RectI xBox(20, 20, 80, 80);
    bm.markForRendered(xBox);
    nonRenderedRects.clear();
    bm.minimalNonMarkedRects(rod, nonRenderedRects);
    EXPECT_TRUE(nonRenderedRects.size() == 4);
    nonRenderedRects.clear();
    bool beingRenderedElseWhere = false;
    bm.minimalNonMarkedRects_trimap(rod, nonRenderedRects, &beingRenderedElseWhere);
    EXPECT_TRUE(nonRenderedRects.size() == 4);
    ASSERT_TRUE(beingRenderedElseWhere == false);

    nonRenderedRects.clear();
    //Mark the A rectangle as being rendered
    RectI aBox(0, 0, 20, 20);
    bm.markForRendering(aBox);
    bm.minimalNonMarkedRects_trimap(rod, nonRenderedRects, &beingRenderedElseWhere);
    ASSERT_TRUE(beingRenderedElseWhere == true);
    EXPECT_TRUE(nonRenderedRects.size() == 3);
} // TEST

TEST(ImageKeyTest, Equality) {
    srand(2000);
    // coverity[dont_call]
    int randomHashKey1 = rand();
    SequenceTime time1 = 0;
    ViewIdx view1(0);
    double pa1 = 1.;
    ImageKey key1(0, randomHashKey1, false, time1, view1, pa1, false, false);
    U64 keyHash1 = key1.getHash();


    ///make a second ImageKey equal to the first
    int randomHashKey2 = randomHashKey1;
    SequenceTime time2 = time1;
    ViewIdx view2(view1);
    double pa2 = pa1;
    ImageKey key2(0, randomHashKey2, false, time2, view2, pa2, false, false);
    U64 keyHash2 = key2.getHash();
    ASSERT_TRUE(keyHash1 == keyHash2);
}

TEST(ImageKeyTest, Difference) {
    srand(2000);
    // coverity[dont_call]
    int randomHashKey1 = rand() % 100;
    SequenceTime time1 = 0;
    ViewIdx view1(0);
    double pa1 = 1.;
    ImageKey key1(0, randomHashKey1, false, time1, view1, pa1, false, false);
    U64 keyHash1 = key1.getHash();


    ///make a second ImageKey different to the first
    // coverity[dont_call]
    int randomHashKey2 = rand() % 1000  + 150;
    SequenceTime time2 = time1;
    ViewIdx view2(view1);
    double pa2 = pa1;
    ImageKey key2(0, randomHashKey2, false, time2, view2, pa2, false, false);
    U64 keyHash2 = key2.getHash();
    ASSERT_TRUE(keyHash1 != keyHash2);
}


// Fill img with a constant RGBA float value on its whole bounds.
static void
fillConstant(Image& img, float v)
{
    img.fill(img.getBounds(), v, v, v, v);
}

// Count the pixels of img inside rect that differ from v (all components).
static int
countDifferent(const Image& img, const RectI& rect, float v)
{
    int n = 0;
    Image::ReadAccess acc( &img );
    for (int y = rect.y1; y < rect.y2; ++y) {
        for (int x = rect.x1; x < rect.x2; ++x) {
            const float* p = (const float*)acc.pixelAt(x, y);
            for (int c = 0; c < 4; ++c) {
                if (p[c] != v) {
                    ++n;
                    break;
                }
            }
        }
    }
    return n;
}

// Bounds that start (or end) left of / below the origin, like the RoD of an
// AdjustRoD node that enlarges an image (see issue #623), at mipmap levels 1-3.
static std::vector<RectI>
negativeTestBounds()
{
    std::vector<RectI> ret;
    const int x1s[] = { -88, -87, -3, -2, -1, 0, 1 };
    const int x2s[] = { 64, 65, 67 };
    for (int x1 : x1s) {
        for (int x2 : x2s) {
            ret.push_back( RectI(x1, x1 - 5, x2, x2 + 3) );
        }
    }
    // entirely in the negative quadrant
    ret.push_back( RectI(-67, -45, -3, -2) );
    ret.push_back( RectI(-68, -46, -4, -1) );

    return ret;
}

TEST(ImageMipmapTest, DownscaleNegativeBounds)
{
    const std::vector<RectI> boundsList = negativeTestBounds();
    for (const RectI& bounds : boundsList) {
        const RectD rod( bounds.x1, bounds.y1, bounds.x2, bounds.y2 );
        Image src(ImagePlaneDesc::getRGBAComponents(), rod, bounds, 0, 1., eImageBitDepthFloat,
                  eImagePremultiplicationPremultiplied, eImageFieldingOrderNone, false);
        fillConstant(src, 0.5f);
        for (unsigned int level = 1; level <= 3; ++level) {
            const RectI dstBounds = rod.toPixelEnclosing(level, 1.);
            ASSERT_EQ( dstBounds, bounds.downscalePowerOfTwoSmallestEnclosing(level) );
            Image dst(ImagePlaneDesc::getRGBAComponents(), rod, dstBounds, level, 1., eImageBitDepthFloat,
                      eImagePremultiplicationPremultiplied, eImageFieldingOrderNone, false);
            fillConstant(dst, -1.f);
            src.downscaleMipmap(rod, bounds, 0, level, false, &dst);
            // downscaleMipmap() only computes the largest enclosed rectangle at the given level (see Image.h):
            // the pixels on the edges of dstBounds that are only partially covered by bounds are not checked.
            const RectI enclosed = bounds.downscalePowerOfTwoLargestEnclosed(level);
            EXPECT_EQ( 0, countDifferent(dst, enclosed, 0.5f) )
                << "bounds=(" << bounds.x1 << "," << bounds.y1 << "," << bounds.x2 << "," << bounds.y2 << ") level=" << level;
        }
    }
}

TEST(ImageMipmapTest, UpscaleNegativeBounds)
{
    const std::vector<RectI> boundsList = negativeTestBounds();
    for (const RectI& fullBounds : boundsList) {
        const RectD rod( fullBounds.x1, fullBounds.y1, fullBounds.x2, fullBounds.y2 );
        for (unsigned int level = 1; level <= 3; ++level) {
            const RectI srcBounds = rod.toPixelEnclosing(level, 1.);
            Image src(ImagePlaneDesc::getRGBAComponents(), rod, srcBounds, level, 1., eImageBitDepthFloat,
                      eImagePremultiplicationPremultiplied, eImageFieldingOrderNone, false);
            fillConstant(src, 0.5f);
            // output covering the whole RoD, and outputs cropped on each side
            std::vector<RectI> dstBoundsList;
            dstBoundsList.push_back(fullBounds);
            dstBoundsList.push_back( RectI(fullBounds.x1 + 3, fullBounds.y1, fullBounds.x2, fullBounds.y2) );
            dstBoundsList.push_back( RectI(fullBounds.x1, fullBounds.y1 + 3, fullBounds.x2, fullBounds.y2) );
            dstBoundsList.push_back( RectI(fullBounds.x1, fullBounds.y1, fullBounds.x2 - 3, fullBounds.y2) );
            dstBoundsList.push_back( RectI(fullBounds.x1 + 5, fullBounds.y1 + 7, fullBounds.x2 - 3, fullBounds.y2 - 1) );
            for (const RectI& dstBounds : dstBoundsList) {
                Image dst(ImagePlaneDesc::getRGBAComponents(), rod, dstBounds, 0, 1., eImageBitDepthFloat,
                          eImagePremultiplicationPremultiplied, eImageFieldingOrderNone, false);
                fillConstant(dst, -1.f);
                src.upscaleMipmap(srcBounds, level, 0, &dst);
                EXPECT_EQ( 0, countDifferent(dst, dstBounds, 0.5f) )
                    << "rod=(" << fullBounds.x1 << "," << fullBounds.y1 << "," << fullBounds.x2 << "," << fullBounds.y2 << ")"
                    << " dst=(" << dstBounds.x1 << "," << dstBounds.y1 << "," << dstBounds.x2 << "," << dstBounds.y2 << ") level=" << level;
            }
        }
    }
}

// Downscaling an image that is one pixel wide goes through Image::halve1DImage().
TEST(ImageMipmapTest, DownscaleOneColumn)
{
    const RectI bounds(-3, -4, -2, 12);
    const RectD rod( bounds.x1, bounds.y1, bounds.x2, bounds.y2 );
    Image src(ImagePlaneDesc::getRGBAComponents(), rod, bounds, 0, 1., eImageBitDepthFloat,
              eImagePremultiplicationPremultiplied, eImageFieldingOrderNone, false);
    for (int y = bounds.y1; y < bounds.y2; ++y) {
        src.fill(RectI(bounds.x1, y, bounds.x2, y + 1), y, y, y, y);
    }
    const RectI dstBounds = bounds.downscalePowerOfTwoSmallestEnclosing(1);
    Image dst(ImagePlaneDesc::getRGBAComponents(), rod, dstBounds, 1, 1., eImageBitDepthFloat,
              eImagePremultiplicationPremultiplied, eImageFieldingOrderNone, false);
    fillConstant(dst, -1.f);
    src.downscaleMipmap(rod, bounds, 0, 1, false, &dst);
    Image::ReadAccess acc( &dst );
    for (int y = dstBounds.y1; y < dstBounds.y2; ++y) {
        const float* p = (const float*)acc.pixelAt(dstBounds.x1, y);
        for (int c = 0; c < 4; ++c) {
            // average of the source rows 2y and 2y+1
            EXPECT_EQ(2 * y + 0.5f, p[c]) << "y=" << y << " c=" << c;
        }
    }
}
