//  Natron
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 * Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 * contact: immarespond at gmail dot com
 *
 */

#include <cstring>
#include <gtest/gtest.h>
#include "Engine/Image.h"


TEST(BitmapTest,SimpleRect) {
    RectI rod(0,0,100,100);
    Natron::Bitmap bm(rod);

    ///assert that the union of all the non rendered rects is the rod
    std::list<RectI> nonRenderedRects = bm.minimalNonMarkedRects(rod);
    RectI nonRenderedRectsUnion;

    for (std::list<RectI>::iterator it = nonRenderedRects.begin(); it != nonRenderedRects.end(); ++it) {
        nonRenderedRectsUnion.merge(*it);
    }

    ASSERT_TRUE(rod == nonRenderedRectsUnion);

    ///assert that the "underlying" bitmap is clean
    const char* map = bm.getBitmap();
    ASSERT_TRUE( !memchr( map,1,rod.area() ) );

    RectI halfRoD(0,0,100,50);
    bm.markForRendered(halfRoD);

    ///assert that non of the rendered rects interesect the non rendered half
    RectI nonRenderedHalf(0,50,100,100);
    nonRenderedRects = bm.minimalNonMarkedRects(rod);
    for (std::list<RectI>::iterator it = nonRenderedRects.begin(); it != nonRenderedRects.end(); ++it) {
        ASSERT_TRUE( (*it).intersects(nonRenderedHalf) );
    }


    ///assert that the underlying bitmap is marked as expected
    const char* start = map;

    ///check that there are only ones in the rendered half
    ASSERT_TRUE( !memchr( start,0,halfRoD.area() ) );

    ///check that there are only 0s in the non rendered half
    start = map + halfRoD.area();
    ASSERT_TRUE( !memchr( start,1,halfRoD.area() ) );

    ///mark for renderer the other half of the rod
    bm.markForRendered(nonRenderedHalf);

    ///assert that the bm is rendered totally
    ASSERT_TRUE( bm.minimalNonMarkedRects(rod).empty() );
    ASSERT_TRUE( !memchr( map,0,rod.area() ) );
}

TEST(ImageKeyTest,Equality) {
    srand(2000);
    int randomHashKey1 = rand();
    SequenceTime time1 = 0;
    int view1 = 0;
    double pa1 = 1.;
    Natron::ImageKey key1(randomHashKey1,time1,view1,pa1);
    U64 keyHash1 = key1.getHash();


    ///make a second ImageKey equal to the first
    int randomHashKey2 = randomHashKey1;
    SequenceTime time2 = time1;
    int view2 = view1;
    double pa2 = pa1;
    Natron::ImageKey key2(randomHashKey2,time2,view2,pa2);
    U64 keyHash2 = key2.getHash();
    ASSERT_TRUE(keyHash1 == keyHash2);
}

TEST(ImageKeyTest,Difference) {
    srand(2000);
    int randomHashKey1 = rand() % 100;
    SequenceTime time1 = 0;
    int view1 = 0;
    double pa1 = 1.;
    Natron::ImageKey key1(randomHashKey1,time1,view1,pa1);
    U64 keyHash1 = key1.getHash();


    ///make a second ImageKey different to the first
    int randomHashKey2 = rand() % 1000  + 150;
    SequenceTime time2 = time1;
    int view2 = view1;
    double pa2 = pa1;
    Natron::ImageKey key2(randomHashKey2,time2,view2,pa2);
    U64 keyHash2 = key2.getHash();
    ASSERT_TRUE(keyHash1 != keyHash2);
}

