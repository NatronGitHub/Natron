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

// The OFX multithread suite's mutexes are counted (recursive) mutexes (ofxMultiThread.h):
// "mutexCreate: Creates a new mutex with lockCount locks on the mutex initially set", and each
// successful lock increases the lock count by one. Natron creates them as QRecursiveMutex, so
// they must also be locked, unlocked and destroyed as QRecursiveMutex: in Qt6 it does not derive
// from QMutex, and using it as a QMutex deadlocks or crashes (#1361).

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <thread>

#include <gtest/gtest.h>

#include <ofxCore.h>
#include <ofxMultiThread.h>

#include "Engine/AppManager.h"
#include "Engine/OfxHost.h"

NATRON_NAMESPACE_USING

namespace {
const OfxMultiThreadSuiteV1*
fetchMultiThreadSuite()
{
    // fetchSuite() is not const, but it does not modify the host
    NATRON_NAMESPACE::OfxHost* host = const_cast<NATRON_NAMESPACE::OfxHost*>( appPTR->getOFXHost() );

    if (!host) {
        return 0;
    }

    return reinterpret_cast<const OfxMultiThreadSuiteV1*>( host->fetchSuite(kOfxMultiThreadSuite, 1) );
}

// Try to lock the mutex from another thread, and unlock it if that succeeded.
bool
tryLockFromOtherThread(const OfxMultiThreadSuiteV1* suite,
                       OfxMutexHandle m)
{
    bool locked = false;
    std::thread t([&]() {
        locked = (suite->mutexTryLock(m) == kOfxStatOK);
        if (locked) {
            suite->mutexUnLock(m);
        }
    });

    t.join();

    return locked;
}
}

TEST(OfxMultiThreadSuite, RecursiveLock)
{
    const OfxMultiThreadSuiteV1* suite = fetchMultiThreadSuite();

    ASSERT_TRUE(suite != 0);

    OfxMutexHandle m = 0;
    ASSERT_EQ(kOfxStatOK, suite->mutexCreate(&m, 0));
    ASSERT_TRUE(m != 0);
    EXPECT_TRUE( tryLockFromOtherThread(suite, m) );

    // relocking from the same thread must not deadlock
    EXPECT_EQ(kOfxStatOK, suite->mutexLock(m));
    EXPECT_EQ(kOfxStatOK, suite->mutexLock(m));
    EXPECT_EQ(kOfxStatOK, suite->mutexTryLock(m));
    EXPECT_FALSE( tryLockFromOtherThread(suite, m) );
    EXPECT_EQ(kOfxStatOK, suite->mutexUnLock(m));
    EXPECT_EQ(kOfxStatOK, suite->mutexUnLock(m));
    // still locked once
    EXPECT_FALSE( tryLockFromOtherThread(suite, m) );
    EXPECT_EQ(kOfxStatOK, suite->mutexUnLock(m));
    EXPECT_TRUE( tryLockFromOtherThread(suite, m) );

    EXPECT_EQ(kOfxStatOK, suite->mutexDestroy(m));
}

TEST(OfxMultiThreadSuite, InitialLockCount)
{
    const OfxMultiThreadSuiteV1* suite = fetchMultiThreadSuite();

    ASSERT_TRUE(suite != 0);

    OfxMutexHandle m = 0;
    ASSERT_EQ(kOfxStatOK, suite->mutexCreate(&m, 2));
    ASSERT_TRUE(m != 0);
    EXPECT_FALSE( tryLockFromOtherThread(suite, m) );
    EXPECT_EQ(kOfxStatOK, suite->mutexUnLock(m));
    EXPECT_FALSE( tryLockFromOtherThread(suite, m) );
    EXPECT_EQ(kOfxStatOK, suite->mutexUnLock(m));
    EXPECT_TRUE( tryLockFromOtherThread(suite, m) );

    EXPECT_EQ(kOfxStatOK, suite->mutexDestroy(m));
}

TEST(OfxMultiThreadSuite, NullHandle)
{
    const OfxMultiThreadSuiteV1* suite = fetchMultiThreadSuite();

    ASSERT_TRUE(suite != 0);

    EXPECT_EQ(kOfxStatErrBadHandle, suite->mutexLock(0));
    EXPECT_EQ(kOfxStatErrBadHandle, suite->mutexUnLock(0));
    EXPECT_EQ(kOfxStatErrBadHandle, suite->mutexTryLock(0));
    EXPECT_EQ(kOfxStatErrBadHandle, suite->mutexDestroy(0));
}
