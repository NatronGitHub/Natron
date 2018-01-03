/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
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
#include <gtest/gtest.h>

#include "Engine/Image.h"
#include "Engine/ImageCacheKey.h"
#include "Engine/ImageCacheEntryProcessing.h"
#include "Engine/CacheEntryKeyBase.h"
#include "Engine/ViewIdx.h"

NATRON_NAMESPACE_USING


TEST(ImageKeyTest, Equality) {
    srand(2000);
    // coverity[dont_call]
    int randomHashKey1 = rand();
    ImageCacheKey key1(randomHashKey1, 0, RenderScale(1.), std::string());
    U64 keyHash1 = key1.getHash();


    ///make a second ImageKey equal to the first
    int randomHashKey2 = randomHashKey1;
    ImageCacheKey key2(randomHashKey2, 0, RenderScale(1.), std::string());
    U64 keyHash2 = key2.getHash();
    ASSERT_TRUE(keyHash1 == keyHash2);
}

TEST(ImageKeyTest, Difference) {
    srand(2000);
    // coverity[dont_call]
    int randomHashKey1 = rand() % 100;
    ImageCacheKey key1(randomHashKey1, 0, RenderScale(1.), std::string());
    U64 keyHash1 = key1.getHash();


    ///make a second ImageKey different to the first
    // coverity[dont_call]
    int randomHashKey2 = rand() % 1000  + 150;
    ImageCacheKey key2(randomHashKey2, 0, RenderScale(1.), std::string());
    U64 keyHash2 = key2.getHash();

    ASSERT_TRUE(keyHash1 != keyHash2);
}

#define getBufAt(x,y) (&buf[roundedBounds.width() * y + x])


TEST(ImageCacheEntryProcessing, RepeatEdgesAbove) {
    /*
     0000
     0000
     1234
     4567
     */
    RectI roundedBounds(0, 0, 4, 4);
    RectI roi(0,0,4,2);
    std::vector<char> buf(roundedBounds.area(), 0);

    *getBufAt(0, 0) = 4; *getBufAt(1, 0) = 5; *getBufAt(2, 0) = 6; *getBufAt(3, 0) = 7;
    *getBufAt(0, 1) = 1; *getBufAt(1, 1) = 2; *getBufAt(2, 1) = 3; *getBufAt(3, 1) = 4;

    ImageCacheEntryProcessing::repeatEdgesForDepth<char>(&buf[0], roi, roundedBounds.width(), roundedBounds.height());

    ASSERT_TRUE(*getBufAt(0,2) == 1); ASSERT_TRUE(*getBufAt(1,2) == 2); ASSERT_TRUE(*getBufAt(2,2) == 3); ASSERT_TRUE(*getBufAt(3,2) == 4);
    ASSERT_TRUE(*getBufAt(0,3) == 1); ASSERT_TRUE(*getBufAt(1,3) == 2); ASSERT_TRUE(*getBufAt(2,3) == 3); ASSERT_TRUE(*getBufAt(3,3) == 4);
}

TEST(ImageCacheEntryProcessing, RepeatEdgesBottom) {
    /*
     1234
     5678
     0000
     0000
     */
    RectI roundedBounds(0, 0, 4, 4);
    RectI roi(0,2,4,4);
    std::vector<char> buf(roundedBounds.area(), 0);

    *getBufAt(0, 2) = 5; *getBufAt(1, 2) = 6; *getBufAt(2, 2) = 7; *getBufAt(3, 2) = 8;
    *getBufAt(0, 3) = 1; *getBufAt(1, 3) = 2; *getBufAt(2, 3) = 3; *getBufAt(3, 3) = 4;

    ImageCacheEntryProcessing::repeatEdgesForDepth<char>(&buf[0], roi, roundedBounds.width(), roundedBounds.height());

    ASSERT_TRUE(*getBufAt(0,0) == 5); ASSERT_TRUE(*getBufAt(1,0) == 6); ASSERT_TRUE(*getBufAt(2,0) == 7); ASSERT_TRUE(*getBufAt(3,0) == 8);
    ASSERT_TRUE(*getBufAt(0,1) == 5); ASSERT_TRUE(*getBufAt(1,1) == 6); ASSERT_TRUE(*getBufAt(2,1) == 7); ASSERT_TRUE(*getBufAt(3,1) == 8);
}

TEST(ImageCacheEntryProcessing, RepeatEdgesRight) {
    /*
     1200
     3400
     5600
     7800
     */
    RectI roundedBounds(0, 0, 4, 4);
    RectI roi(0,0,2,4);
    std::vector<char> buf(roundedBounds.area(), 0);

    *getBufAt(0, 0) = 7; *getBufAt(1, 0) = 8;
    *getBufAt(0, 1) = 5; *getBufAt(1, 1) = 6;
    *getBufAt(0, 2) = 3; *getBufAt(1, 2) = 4;
    *getBufAt(0, 3) = 1; *getBufAt(1, 3) = 2;

    ImageCacheEntryProcessing::repeatEdgesForDepth<char>(&buf[0], roi, roundedBounds.width(), roundedBounds.height());

    ASSERT_TRUE(*getBufAt(2,0) == 8); ASSERT_TRUE(*getBufAt(3,0) == 8);
    ASSERT_TRUE(*getBufAt(2,1) == 6); ASSERT_TRUE(*getBufAt(3,1) == 6);
    ASSERT_TRUE(*getBufAt(2,2) == 4); ASSERT_TRUE(*getBufAt(3,2) == 4);
    ASSERT_TRUE(*getBufAt(2,3) == 2); ASSERT_TRUE(*getBufAt(3,3) == 2);
}

TEST(ImageCacheEntryProcessing, RepeatEdgesLeft) {
    /*
     0012
     0034
     0056
     0078
     */
    RectI roundedBounds(0, 0, 4, 4);
    RectI roi(2,0,4,4);
    std::vector<char> buf(roundedBounds.area(), 0);

    *getBufAt(2, 0) = 7; *getBufAt(3, 0) = 8;
    *getBufAt(2, 1) = 5; *getBufAt(3, 1) = 6;
    *getBufAt(2, 2) = 3; *getBufAt(3, 2) = 4;
    *getBufAt(2, 3) = 1; *getBufAt(3, 3) = 2;

    ImageCacheEntryProcessing::repeatEdgesForDepth<char>(&buf[0], roi, roundedBounds.width(), roundedBounds.height());

    ASSERT_TRUE(*getBufAt(0,0) == 7); ASSERT_TRUE(*getBufAt(1,0) == 7);
    ASSERT_TRUE(*getBufAt(0,1) == 5); ASSERT_TRUE(*getBufAt(1,1) == 5);
    ASSERT_TRUE(*getBufAt(0,2) == 3); ASSERT_TRUE(*getBufAt(1,2) == 3);
    ASSERT_TRUE(*getBufAt(0,3) == 1); ASSERT_TRUE(*getBufAt(1,3) == 1);
}

TEST(ImageCacheEntryProcessing, RepeatEdgesBottomLeft) {
    /*
     0000
     0000
     3400
     1200
     */
    RectI roundedBounds(0, 0, 4, 4);
    RectI roi(0,0,2,2);
    std::vector<char> buf(roundedBounds.area(), 0);

    *getBufAt(0, 0) = 1; *getBufAt(1, 0) = 2;
    *getBufAt(0, 1) = 3; *getBufAt(1, 1) = 4;

    ImageCacheEntryProcessing::repeatEdgesForDepth<char>(&buf[0], roi, roundedBounds.width(), roundedBounds.height());

    ASSERT_TRUE(*getBufAt(2,0) == 2); ASSERT_TRUE(*getBufAt(3,0) == 2);
    ASSERT_TRUE(*getBufAt(2,1) == 4); ASSERT_TRUE(*getBufAt(3,1) == 4);

    ASSERT_TRUE(*getBufAt(2,2) == 4); ASSERT_TRUE(*getBufAt(3,2) == 4);
    ASSERT_TRUE(*getBufAt(2,3) == 4); ASSERT_TRUE(*getBufAt(3,3) == 4);

    ASSERT_TRUE(*getBufAt(0,2) == 3); ASSERT_TRUE(*getBufAt(1,2) == 4);
    ASSERT_TRUE(*getBufAt(0,3) == 3); ASSERT_TRUE(*getBufAt(1,3) == 4);
}

TEST(ImageCacheEntryProcessing, RepeatEdgesBottomRight) {
    /*
     0000
     0000
     0034
     0012
     */
    RectI roundedBounds(0, 0, 4, 4);
    RectI roi(2,0,4,2);
    std::vector<char> buf(roundedBounds.area(), 0);

    *getBufAt(2, 0) = 1; *getBufAt(3, 0) = 2;
    *getBufAt(2, 1) = 3; *getBufAt(3, 1) = 4;

    ImageCacheEntryProcessing::repeatEdgesForDepth<char>(&buf[0], roi, roundedBounds.width(), roundedBounds.height());

    ASSERT_TRUE(*getBufAt(2,2) == 3); ASSERT_TRUE(*getBufAt(3,2) == 4);
    ASSERT_TRUE(*getBufAt(2,3) == 3); ASSERT_TRUE(*getBufAt(3,3) == 4);

    ASSERT_TRUE(*getBufAt(0,2) == 3); ASSERT_TRUE(*getBufAt(0,2) == 3);
    ASSERT_TRUE(*getBufAt(0,3) == 3); ASSERT_TRUE(*getBufAt(0,3) == 3);

    ASSERT_TRUE(*getBufAt(0,0) == 1); ASSERT_TRUE(*getBufAt(1,0) == 1);
    ASSERT_TRUE(*getBufAt(0,1) == 3); ASSERT_TRUE(*getBufAt(1,1) == 3);
}

TEST(ImageCacheEntryProcessing, RepeatEdgesTopRight) {
    /*
     0034
     0012
     0000
     0000
     */
    RectI roundedBounds(0, 0, 4, 4);
    RectI roi(2,2,4,4);
    std::vector<char> buf(roundedBounds.area(), 0);

    *getBufAt(2, 2) = 1; *getBufAt(3, 2) = 2;
    *getBufAt(2, 3) = 3; *getBufAt(3, 3) = 4;

    ImageCacheEntryProcessing::repeatEdgesForDepth<char>(&buf[0], roi, roundedBounds.width(), roundedBounds.height());

    ASSERT_TRUE(*getBufAt(2,0) == 1); ASSERT_TRUE(*getBufAt(3,0) == 2);
    ASSERT_TRUE(*getBufAt(2,1) == 1); ASSERT_TRUE(*getBufAt(3,1) == 2);

    ASSERT_TRUE(*getBufAt(0,0) == 1); ASSERT_TRUE(*getBufAt(0,1) == 1);
    ASSERT_TRUE(*getBufAt(0,1) == 1); ASSERT_TRUE(*getBufAt(1,1) == 1);

    ASSERT_TRUE(*getBufAt(2,0) == 1); ASSERT_TRUE(*getBufAt(3,0) == 2);
    ASSERT_TRUE(*getBufAt(2,1) == 1); ASSERT_TRUE(*getBufAt(3,1) == 2);
}

TEST(ImageCacheEntryProcessing, RepeatEdgesTopLeft) {
    /*
     3400
     1200
     0000
     0000
     */
    RectI roundedBounds(0, 0, 4, 4);
    RectI roi(0,2,2,4);
    std::vector<char> buf(roundedBounds.area(), 0);

    *getBufAt(0, 2) = 1; *getBufAt(1, 2) = 2;
    *getBufAt(0, 3) = 3; *getBufAt(1, 3) = 4;

    ImageCacheEntryProcessing::repeatEdgesForDepth<char>(&buf[0], roi, roundedBounds.width(), roundedBounds.height());

    ASSERT_TRUE(*getBufAt(2,2) == 2); ASSERT_TRUE(*getBufAt(3,2) == 2);
    ASSERT_TRUE(*getBufAt(2,3) == 4); ASSERT_TRUE(*getBufAt(3,3) == 4);

    ASSERT_TRUE(*getBufAt(2,0) == 2); ASSERT_TRUE(*getBufAt(3,0) == 2);
    ASSERT_TRUE(*getBufAt(2,1) == 2); ASSERT_TRUE(*getBufAt(3,1) == 2);

    ASSERT_TRUE(*getBufAt(0,0) == 1); ASSERT_TRUE(*getBufAt(1,0) == 2);
    ASSERT_TRUE(*getBufAt(0,1) == 1); ASSERT_TRUE(*getBufAt(1,1) == 2);
}

TEST(ImageCacheEntryProcessing, RepeatEdgesGeneralCase) {
    /*
     Make such a rectangle
     00000
     01230
     04560
     07890
     00000

     fill the 0s by their corresponding numbers
     */
    RectI roundedBounds(0, 0, 5, 5);
    RectI roi(1, 1, 4, 4);
    std::vector<char> buf(roundedBounds.area(), 0);


    *getBufAt(1, 3) = 1; *getBufAt(2, 3) = 2; *getBufAt(3, 3) = 3;
    *getBufAt(1, 2) = 4; *getBufAt(2, 2) = 5; *getBufAt(3, 2) = 6;
    *getBufAt(1, 1) = 7; *getBufAt(2, 1) = 8; *getBufAt(3, 1) = 9;

    ImageCacheEntryProcessing::repeatEdgesForDepth<char>(&buf[0], roi, roundedBounds.width(), roundedBounds.height());

    ASSERT_TRUE(*getBufAt(0,0) == 7);
    ASSERT_TRUE(*getBufAt(1,0) == 7);
    ASSERT_TRUE(*getBufAt(2,0) == 8);
    ASSERT_TRUE(*getBufAt(3,0) == 9);
    ASSERT_TRUE(*getBufAt(4,0) == 9);

    ASSERT_TRUE(*getBufAt(0,1) == 7);
    ASSERT_TRUE(*getBufAt(4,1) == 9);

    ASSERT_TRUE(*getBufAt(0,2) == 4);
    ASSERT_TRUE(*getBufAt(4,2) == 6);

    ASSERT_TRUE(*getBufAt(0,3) == 1);
    ASSERT_TRUE(*getBufAt(4,3) == 3);

    ASSERT_TRUE(*getBufAt(0,4) == 1);
    ASSERT_TRUE(*getBufAt(1,4) == 1);
    ASSERT_TRUE(*getBufAt(2,4) == 2);
    ASSERT_TRUE(*getBufAt(3,4) == 3);
    ASSERT_TRUE(*getBufAt(4,4) == 3);
}


#undef getBufAt


