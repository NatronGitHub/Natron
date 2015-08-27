/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2015 INRIA and Alexandre Gauthier
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

#include <cstdlib>
#include <gtest/gtest.h>
#include "Engine/Hash64.h"

TEST(Hash64,GeneralTest) {
    Hash64 hash1;

    ASSERT_FALSE( hash1.valid() ) << "A fresh hash is not valid.";

    hash1.computeHash();
    ASSERT_FALSE( hash1.valid() ) << "Compute on an empty hash doesn't make it valid";

    hash1.append<int>(3);
    ASSERT_FALSE( hash1.valid() ) << "You need to call compute to make the hash valid";

    hash1.computeHash();
    ASSERT_TRUE( hash1.valid() );

    hash1.reset();
    ASSERT_FALSE( hash1.valid() );

    srand(2000);
    // coverity[dont_call]
    int hash1ElementCount = rand() % 100 + 80;
    std::vector<int> hash1Elements;
    for (int i = 0; i < hash1ElementCount; ++i) {
        // coverity[dont_call]
        int v = rand();
        hash1Elements.push_back(v);
        hash1.append<int>(v);
    }

    hash1.computeHash();
    ASSERT_TRUE( hash1.valid() );

    Hash64 hash2;
    for (int i = 0; i < hash1ElementCount; ++i) {
        hash2.append<int>(hash1Elements[i]);
    }
    hash2.computeHash();
    ASSERT_TRUE( hash2.valid() );

    EXPECT_EQ( hash1.value(), hash2.value() ) << "Hashs with same elements should be equal.";
    EXPECT_EQ(hash1, hash2);

    hash2.reset();
    ASSERT_FALSE( hash2.valid() );

    // coverity[dont_call]
    int hash2ElementCount = hash1ElementCount + (rand() % 10 + 1);
    for (int i = 0; i < hash2ElementCount; ++i) {
        // coverity[dont_call]
        hash2.append<int>( rand() );
    }
    hash2.computeHash();
    ASSERT_TRUE( hash2.valid() );

    EXPECT_NE( hash1.value(), hash2.value() );
    EXPECT_NE(hash1, hash2);
}
