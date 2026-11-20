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

// OfxImageEffectSuiteV1::imageMemoryAlloc (ofxImageEffect.h) returns "kOfxStatErrMemory if there
// was not enough memory to satisfy the call, memoryHandle is set to NULL". Natron used to return
// kOfxStatOK with a handle to no memory, after opening an "Out of memory" dialog, possibly from a
// render thread (#1365).

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <cstring>
#include <limits>

#include <gtest/gtest.h>

#include <ofxCore.h>
#include <ofxImageEffect.h>

#include "Engine/AppManager.h"
#include "Engine/OfxHost.h"

NATRON_NAMESPACE_USING

namespace {
const OfxImageEffectSuiteV1*
fetchImageEffectSuite()
{
    // fetchSuite() is not const, but it does not modify the host
    NATRON_NAMESPACE::OfxHost* host = const_cast<NATRON_NAMESPACE::OfxHost*>( appPTR->getOFXHost() );

    if (!host) {
        return 0;
    }

    return reinterpret_cast<const OfxImageEffectSuiteV1*>( host->fetchSuite(kOfxImageEffectSuite, 1) );
}
}

TEST(OfxImageMemory, AllocLockFree)
{
    const OfxImageEffectSuiteV1* suite = fetchImageEffectSuite();

    ASSERT_TRUE(suite != 0);

    // a NULL instance handle means memory that is not tied to an effect instance
    OfxImageMemoryHandle h = 0;
    ASSERT_EQ( kOfxStatOK, suite->imageMemoryAlloc(0, 1024, &h) );
    ASSERT_TRUE(h != 0);
    void* ptr = 0;
    ASSERT_EQ( kOfxStatOK, suite->imageMemoryLock(h, &ptr) );
    ASSERT_TRUE(ptr != 0);
    std::memset(ptr, 0, 1024);
    EXPECT_EQ( kOfxStatOK, suite->imageMemoryUnlock(h) );
    EXPECT_EQ( kOfxStatOK, suite->imageMemoryFree(h) );
}

TEST(OfxImageMemory, AllocFailure)
{
    const OfxImageEffectSuiteV1* suite = fetchImageEffectSuite();

    ASSERT_TRUE(suite != 0);

    // more than any address space can hold
    const size_t nBytes = std::numeric_limits<size_t>::max() / 2;
    OfxImageMemoryHandle h = reinterpret_cast<OfxImageMemoryHandle>(&h); // not NULL
    EXPECT_EQ( kOfxStatErrMemory, suite->imageMemoryAlloc(0, nBytes, &h) );
    EXPECT_TRUE(h == 0);
}
