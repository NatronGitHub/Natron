//  Natron
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <cstdlib>
#include <gtest/gtest.h>
#include "Engine/Lut.h"

using namespace Natron::Color;

TEST(Lut,IntConversions) {
    for(int i=0; i < 0x10000; ++i) {
        //printf("%x -> %x,%x\n", i, uint16ToChar(i), floatToInt<256>(intToFloat<65536>(i)));
        EXPECT_EQ (uint16ToChar(i), floatToInt<256>(intToFloat<65536>(i)));
    }
    for(int i=0; i < 0x100; ++i) {
        //printf("%x -> %x,%x\n", i, charToUint16(i), floatToInt<65536>(intToFloat<256>(i)));
        EXPECT_EQ(charToUint16(i), floatToInt<65536>(intToFloat<256>(i)));
        EXPECT_EQ(i, uint16ToChar(charToUint16(i)));
    }
    for(int i=0; i < 0xff01; ++i) {
        //printf("%x -> %x,%x, err=%d\n", i, uint8xxToChar(i), floatToInt<256>(intToFloat<0xff01>(i)),i - charToUint8xx(uint8xxToChar(i)));
        EXPECT_EQ(uint8xxToChar(i), floatToInt<256>(intToFloat<0xff01>(i)));
    }
    for(int i=0; i < 0x100; ++i) {
        //printf("%x -> %x,%x\n", i, charToUint8xx(i), floatToInt<0xff01>(intToFloat<256>(i)));
        EXPECT_EQ(charToUint8xx(i), floatToInt<0xff01>(intToFloat<256>(i)));
        EXPECT_EQ(i, uint8xxToChar(charToUint8xx(i)));
    }
}
