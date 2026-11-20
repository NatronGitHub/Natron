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

// Stress tests for the lock-order inversions of #1543, which could deadlock two threads:
// - a Curve change (Curve::_lock, then the knob's expression results) against
//   Knob<T>::dequeueValuesSet() (the knob's _valueMutex, then Curve::_lock);
// - A.clone(B) against B.clone(A) (each locked its own _valueMutex, then the other knob's).
// Each test runs both sides in two threads; a watchdog aborts the test binary if they don't finish,
// so a deadlock fails the test instead of hanging it. A ThreadSanitizer build also reports the
// inversion itself ("lock-order-inversion"), even when the threads don't actually deadlock.

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <thread>

#include <gtest/gtest.h>

#include "Engine/AppInstance.h"
#include "Engine/AppManager.h"
#include "Engine/Curve.h"
#include "Engine/Knob.h"
#include "Engine/KnobTypes.h"
#include "Engine/ViewIdx.h"

NATRON_NAMESPACE_USING

namespace {
// A knob holder whose setValue() calls can be queued, as those of a node during a render
// (EffectInstance::canSetValue()): they are applied later by dequeueValuesSet().
class LockOrderTestHolder
    : public KnobHolder
{
public:
    LockOrderTestHolder()
        : KnobHolder( appPTR->getTopLevelInstance() )
        , _canSet(true)
    {
    }

    virtual void initializeKnobs() OVERRIDE FINAL {}

    void setCanSetValue(bool canSet) { _canSet = canSet; }

protected:
    virtual bool canSetValue() const OVERRIDE FINAL { return _canSet; }

private:
    std::atomic<bool> _canSet;
};

// Run f1 and f2 in two threads; abort the process if they don't finish within timeoutSeconds (deadlock).
template <typename F1, typename F2>
void
runConcurrently(const char* what,
                F1 f1,
                F2 f2,
                int timeoutSeconds = 120)
{
    std::atomic<int> done(0);
    std::thread t1([&] { f1(); ++done; });
    std::thread t2([&] { f2(); ++done; });
    const std::chrono::steady_clock::time_point deadline = std::chrono::steady_clock::now() + std::chrono::seconds(timeoutSeconds);
    while (done < 2) {
        if (std::chrono::steady_clock::now() > deadline) {
            std::fprintf(stderr, "%s: the two threads did not finish within %d s (deadlock, #1543)\n", what, timeoutSeconds);
            std::fflush(stderr);
            std::abort();
        }
        std::this_thread::sleep_for( std::chrono::milliseconds(10) );
    }
    t1.join();
    t2.join();
}

const int kIterations = 20000;
} // anonymous namespace

// Thread 1 queues keyframes and dequeues them (Knob::_valueMutex, then Curve::_lock); thread 2 replaces
// the curve (Curve::_lock, then Curve::onCurveChanged() -> Knob::clearExpressionsResults()).
TEST(KnobLockOrder, CurveChangeWhileDequeuingKeyframes)
{
    LockOrderTestHolder holder;
    KnobDoublePtr knob = AppManager::createKnob<KnobDouble>(&holder, std::string("value"), 1);
    knob->setName("value");
    // animate the curve, then queue the following setValueAtTime() calls
    knob->setValueAtTime(0., 0., ViewSpec::all(), 0);
    ASSERT_EQ( 1, knob->getKeyFramesCount(ViewIdx(0), 0) );
    holder.setCanSetValue(false);

    CurvePtr curve = knob->getCurve(ViewIdx(0), 0, true /*byPassMaster*/);
    ASSERT_TRUE( bool(curve) );
    Curve source;
    source.addKeyFrame( KeyFrame(0., 1.) );
    source.addKeyFrame( KeyFrame(10., 2.) );

    runConcurrently("CurveChangeWhileDequeuingKeyframes",
                    [&] {
        for (int i = 0; i < kIterations; ++i) {
            knob->setValueAtTime(1 + i % 8, (double)i, ViewSpec::all(), 0);
            knob->dequeueValuesSet(true /*disableEvaluation*/);
        }
    },
                    [&] {
        for (int i = 0; i < kIterations; ++i) {
            curve->clone(source);
            (void)knob->getValueAtTime(1 + i % 8, 0);
        }
    });

    holder.setCanSetValue(true);
    knob->dequeueValuesSet(true);
    EXPECT_GE(curve->getKeyFramesCount(), 2);
    knob.reset();
}

// A.clone(B) and B.clone(A) at the same time.
TEST(KnobLockOrder, CloneBothWays)
{
    LockOrderTestHolder holder;
    KnobDoublePtr a = AppManager::createKnob<KnobDouble>(&holder, std::string("a"), 2);
    a->setName("a");
    KnobDoublePtr b = AppManager::createKnob<KnobDouble>(&holder, std::string("b"), 2);
    b->setName("b");
    a->setValue(1., ViewSpec::all(), 0);
    a->setValue(2., ViewSpec::all(), 1);
    b->setValue(3., ViewSpec::all(), 0);
    b->setValue(4., ViewSpec::all(), 1);

    runConcurrently("CloneBothWays",
                    [&] {
        for (int i = 0; i < kIterations; ++i) {
            if (i % 2) {
                a->clone( b.get() );
            } else {
                ignore_result( a->cloneAndCheckIfChanged( b.get(), i % 4 == 0 ? -1 : 1, i % 4 == 0 ? -1 : 1 ) );
            }
        }
    },
                    [&] {
        for (int i = 0; i < kIterations; ++i) {
            if (i % 2) {
                b->clone( a.get() );
            } else {
                ignore_result( b->cloneAndCheckIfChanged( a.get(), i % 4 == 0 ? -1 : 0, i % 4 == 0 ? -1 : 0 ) );
            }
        }
    });

    // each dimension holds one of the initial values of that dimension
    EXPECT_TRUE(a->getValue(0) == 1. || a->getValue(0) == 3.);
    EXPECT_TRUE(b->getValue(1) == 2. || b->getValue(1) == 4.);
    a.reset();
    b.reset();
}
