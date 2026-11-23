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

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <atomic>
#include <cstdlib>
#include <chrono>
#include <future>
#include <memory>

#include <gtest/gtest.h>

#include <QThread>

#include "Engine/GenericSchedulerThread.h"

NATRON_NAMESPACE_USING

namespace {

// Works until it is aborted, then takes a while to finish, like a render that must clean up.
class SlowAbortThread
    : public GenericSchedulerThread
{
public:
    std::atomic<bool> started{false};
    std::atomic<bool> finished{false};

    virtual ~SlowAbortThread() OVERRIDE FINAL
    {
        quitThread(false);
        wait();
    }

private:
    virtual TaskQueueBehaviorEnum tasksQueueBehaviour() const OVERRIDE FINAL
    {
        return eTaskQueueBehaviorProcessInOrder;
    }

    virtual ThreadStateEnum threadLoopOnce(const GenericThreadStartArgsPtr& /*inArgs*/) OVERRIDE FINAL
    {
        started = true;
        ThreadStateEnum state;
        while ( ( state = resolveState() ) == eThreadStateActive ) {
            QThread::msleep(1);
        }
        QThread::msleep(200);
        finished = true;

        return state;
    }
};

bool
waitUntilStarted(const SlowAbortThread& thread)
{
    for (int i = 0; i < 5000 && !thread.started; ++i) {
        QThread::msleep(1);
    }

    return thread.started;
}

} // anon namespace

// waitForAbortToComplete_enforce_blocking() returned at once, before the abort was processed (#1617)
TEST(GenericSchedulerThread, WaitForAbortWaits)
{
    SlowAbortThread thread;

    ASSERT_TRUE( thread.startTask( std::make_shared<GenericThreadStartArgs>() ) );
    ASSERT_TRUE( waitUntilStarted(thread) );

    ASSERT_TRUE( thread.abortThreadedTask() );
    EXPECT_TRUE( thread.waitForAbortToComplete_enforce_blocking() );
    EXPECT_TRUE(thread.finished);
    EXPECT_FALSE( thread.isBeingAborted() );
}

// quitThread() aborts the active task; the thread then stops: a thread waiting for that abort must be woken
TEST(GenericSchedulerThread, WaitForAbortReturnsWhenThreadQuits)
{
    SlowAbortThread thread;

    ASSERT_TRUE( thread.startTask( std::make_shared<GenericThreadStartArgs>() ) );
    ASSERT_TRUE( waitUntilStarted(thread) );

    ASSERT_TRUE( thread.quitThread(false) );
    std::future<bool> waited = std::async(std::launch::async, [&thread] {
        return thread.waitForAbortToComplete_enforce_blocking();
    });
    if (waited.wait_for( std::chrono::seconds(10) ) != std::future_status::ready) {
        // The waiter can never return: don't hang the whole test binary
        ADD_FAILURE() << "waitForAbortToComplete_enforce_blocking() did not return after the thread quit";
        std::abort();
    }
    waited.get();
    EXPECT_TRUE(thread.finished);
    EXPECT_FALSE( thread.isBeingAborted() );
}
