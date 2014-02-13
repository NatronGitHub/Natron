//  Natron
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 *Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 *contact: immarespond at gmail dot com
 *
 */

#include <cstdlib>
#include <gtest/gtest.h>
#include "Engine/Hash64.h"

TEST(Hash64,GeneralTest) {
    Hash64 hash1;
    ASSERT_FALSE(hash1.valid()) << "A fresh hash is not valid.";

    hash1.computeHash();
    ASSERT_FALSE(hash1.valid()) << "Compute on an empty hash doesn't make it valid";

    hash1.append<int>(3);
    ASSERT_FALSE(hash1.valid()) << "You need to call compute to make the hash valid";

    hash1.computeHash();
    ASSERT_TRUE(hash1.valid());

    hash1.reset();
    ASSERT_FALSE(hash1.valid());

    srand(2000);
    int hash1ElementCount = rand() % 100 + 80;
    std::vector<int> hash1Elements;
    for (int i = 0 ; i < hash1ElementCount ; ++i) {
        int v = rand();
        hash1Elements.push_back(v);
        hash1.append<int>(v);
    }

    hash1.computeHash();
    ASSERT_TRUE(hash1.valid());

    Hash64 hash2;
    for (int i = 0 ; i < hash1ElementCount ; ++i) {
        hash2.append<int>(hash1Elements[i]);
    }
    hash2.computeHash();
    ASSERT_TRUE(hash2.valid());

    EXPECT_EQ(hash1.value(), hash2.value()) << "Hashs with same elements should be equal.";
    EXPECT_EQ(hash1, hash2);

    hash2.reset();
    ASSERT_FALSE(hash2.valid());

    int hash2ElementCount = hash1ElementCount + (rand() % 10 + 1);
    for (int i = 0; i < hash2ElementCount;++i) {
        hash2.append<int>(rand());
    }
    hash2.computeHash();
    ASSERT_TRUE(hash2.valid());

    EXPECT_NE(hash1.value(), hash2.value());
    EXPECT_NE(hash1, hash2);

}
