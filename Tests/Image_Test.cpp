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

#include "Global/Macros.h"

#include <cstring>
#include <gtest/gtest.h>

#include "Engine/Image.h"
#include "Engine/ImageCacheKey.h"
#include "Engine/CacheEntryKeyBase.h"
#include "Engine/ViewIdx.h"

NATRON_NAMESPACE_USING

TEST(ImageKeyTest, Equality) {
    srand(2000);
    // coverity[dont_call]
    int randomHashKey1 = rand();
    ImageCacheKey key1(randomHashKey1, 0, RenderScale(1.), false, std::string());
    U64 keyHash1 = key1.getHash();


    ///make a second ImageKey equal to the first
    int randomHashKey2 = randomHashKey1;
    ImageCacheKey key2(randomHashKey2, 0, RenderScale(1.), false, std::string());
    U64 keyHash2 = key2.getHash();
    ASSERT_TRUE(keyHash1 == keyHash2);
}

TEST(ImageKeyTest, Difference) {
    srand(2000);
    // coverity[dont_call]
    int randomHashKey1 = rand() % 100;
    ImageCacheKey key1(randomHashKey1, 0, RenderScale(1.), false, std::string());
    U64 keyHash1 = key1.getHash();


    ///make a second ImageKey different to the first
    // coverity[dont_call]
    int randomHashKey2 = rand() % 1000  + 150;
    ImageCacheKey key2(randomHashKey2, 0, RenderScale(1.), false, std::string());
    U64 keyHash2 = key2.getHash();

    ASSERT_TRUE(keyHash1 != keyHash2);
}

