/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2016 INRIA and Alexandre Gauthier-Foichat
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
#include "Engine/Lut.h"

using namespace Natron::Color;

TEST(Lut,IntConversions) {
    for (int i = 0; i < 0x10000; ++i) {
        //printf("%x -> %x,%x\n", i, uint16ToChar(i), floatToInt<256>(intToFloat<65536>(i)));
        EXPECT_EQ ( uint16ToChar(i), floatToInt<256>( intToFloat<65536>(i) ) );
    }
    for (int i = 0; i < 0x100; ++i) {
        //printf("%x -> %x,%x\n", i, charToUint16(i), floatToInt<65536>(intToFloat<256>(i)));
        EXPECT_EQ( charToUint16(i), floatToInt<65536>( intToFloat<256>(i) ) );
        EXPECT_EQ( i, uint16ToChar( charToUint16(i) ) );
    }
    for (int i = 0; i < 0xff01; ++i) {
        //printf("%x -> %x,%x, err=%d\n", i, uint8xxToChar(i), floatToInt<256>(intToFloat<0xff01>(i)),i - charToUint8xx(uint8xxToChar(i)));
        EXPECT_EQ( uint8xxToChar(i), floatToInt<256>( intToFloat<0xff01>(i) ) );
    }
    for (int i = 0; i < 0x100; ++i) {
        //printf("%x -> %x,%x\n", i, charToUint8xx(i), floatToInt<0xff01>(intToFloat<256>(i)));
        EXPECT_EQ( charToUint8xx(i), floatToInt<0xff01>( intToFloat<256>(i) ) );
        EXPECT_EQ( i, uint8xxToChar( charToUint8xx(i) ) );
    }
}
