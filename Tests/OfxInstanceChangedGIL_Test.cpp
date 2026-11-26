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

// A Python script that presses a button or sets a parameter holds the Python GIL while the plug-in's
// kOfxActionInstanceChanged runs. If that action waits for threads that need the GIL, e.g. to evaluate a
// Python expression, Natron deadlocks (#1653): when it fetches an input image whose render waits for a
// render lock held by another render, or when it runs threads of the multi-thread suite. The plug-ins are
// in Tests/OfxTestPlugin/NatronTestPluginGILWait.cpp.

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <atomic>
#include <chrono>
#include <cstdlib>
#include <functional>
#include <future>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include <gtest/gtest.h>

#include <QThreadPool>

#include "Engine/AppInstance.h"
#include "Engine/AppManager.h"
#include "Engine/KnobTypes.h"
#include "Engine/Node.h"
#include "Engine/Project.h"
#include "Engine/TLSHolder.h"

#include "OfxTestPlugin.h"

#define kGILWaitSource "net.natron.test.GILWaitSource"
#define kGILWaitAnalysis "net.natron.test.GILWaitAnalysis"

NATRON_NAMESPACE_USING

namespace {
// If the threads deadlock, nothing else can report it: abort after a while.
class Watchdog
{
    std::atomic<bool> _done;
    std::thread _thread;

public:
    explicit Watchdog(const std::string& testName)
        : _done(false)
    {
        _thread = std::thread([this, testName]() {
            for (int i = 0; i < 600 && !_done; ++i) {
                std::this_thread::sleep_for( std::chrono::milliseconds(100) );
            }
            if (!_done) {
                std::cerr << testName << ": deadlock (#1653)? (total time of 60 s exceeded)" << std::endl;
                std::abort();
            }
        });
    }

    ~Watchdog()
    {
        _done = true;
        if ( _thread.joinable() ) {
            _thread.join();
        }
    }
};

// Joins its thread when destroyed, also when the test exits early (CP.25).
class JoiningThread
{
    std::thread _thread;

public:
    explicit JoiningThread(const std::function<void()>& f)
        : _thread(f)
    {
    }

    ~JoiningThread()
    {
        if ( _thread.joinable() ) {
            _thread.join();
        }
    }
};

// Give the "value" parameter of node a Python expression: reading it needs the GIL.
bool
setValueExpression(const NodePtr& node)
{
    KnobDoublePtr value = std::dynamic_pointer_cast<KnobDouble>( node->getKnobByName("value") );

    if (!value) {
        return false;
    }
    try {
        value->setExpression(0, "frame * 2", false /*hasRetVariable*/, true /*failIfInvalid*/);
    } catch (const std::exception& e) {
        std::cerr << "setExpression: " << e.what() << std::endl;

        return false;
    }

    return true;
}

// Press the button of node from Python, as a script does: the GIL is held during kOfxActionInstanceChanged.
bool
pressButtonFromPython(const NodePtr& node,
                      const std::string& button)
{
    std::string script = node->getApp()->getAppIDString() + '.' + node->getFullyQualifiedName() + '.' + button + ".trigger()\n";
    std::string error;
    bool ok = NATRON_NAMESPACE::NATRON_PYTHON_NAMESPACE::interpretPythonScript(script, &error, 0);

    EXPECT_TRUE(ok) << error;

    return ok;
}

// What the tests leave behind must not block the tests that follow (on Windows CI, a render test that ran
// after them hung, OfxBytesParamTest.RenderAction): this thread must not hold the GIL, another thread
// must be able to take it, and the threads of the global thread pool that the test used must be idle again.
// A GIL that stays taken would hang the rest of the binary: abort with a message instead.
void
checkNothingLeftBehind(const char* testName,
                       int activeThreadsBefore)
{
    {
        // The thread-local data that the test's actions left on this thread may hold the last references to
        // the effects of the destroyed nodes: cleaning it up (as Node::makePreviewImage() does) destroys them,
        // which runs their kOfxActionDestroyInstance.
        Watchdog watchdog( std::string(testName) + ": AppTLS::cleanupTLSForThread()" );
        appPTR->getAppTLS()->cleanupTLSForThread();
    }
    EXPECT_FALSE( PyGILState_Check() ) << testName << ": the calling thread still holds the GIL";

    std::packaged_task<void()> takeGIL([]() {
        PythonGILLocker pgl;
    });
    std::future<void> tookGIL = takeGIL.get_future();
    std::thread gilThread( std::move(takeGIL) );
    if (tookGIL.wait_for( std::chrono::seconds(10) ) != std::future_status::ready) {
        std::cerr << testName << ": another thread cannot take the GIL after the test (10 s)" << std::endl;
        std::abort();
    }
    gilThread.join();

    int activeThreads = QThreadPool::globalInstance()->activeThreadCount();
    for (int i = 0; i < 1000 && activeThreads > activeThreadsBefore; ++i) {
        std::this_thread::sleep_for( std::chrono::milliseconds(10) );
        activeThreads = QThreadPool::globalInstance()->activeThreadCount();
    }
    EXPECT_LE(activeThreads, activeThreadsBefore) << testName << ": threads of the global thread pool are still busy";
}

// Destroys the test's nodes and checks what the test left behind when the test ends, also when an ASSERT_*
// returns early, so that no node with a Python expression stays in the shared project (E.19).
class TestCleanup
{
    const char* _testName;
    const int _activeThreadsBefore;

public:
    std::vector<NodePtr> nodes; // destroyed in reverse order

    explicit TestCleanup(const char* testName)
        : _testName(testName)
        , _activeThreadsBefore( QThreadPool::globalInstance()->activeThreadCount() )
    {
    }

    ~TestCleanup()
    {
        for (std::vector<NodePtr>::reverse_iterator it = nodes.rbegin(); it != nodes.rend(); ++it) {
            if (*it) {
                (*it)->destroyNode(true /*blockingDestroy*/, false /*autoReconnect*/);
            }
        }
        nodes.clear();
        checkNothingLeftBehind(_testName, _activeThreadsBefore);
    }

    TestCleanup(const TestCleanup&) = delete;
    TestCleanup& operator=(const TestCleanup&) = delete;
};
} // anonymous namespace

// The script presses "fetch" while another thread renders the input: that render holds the input's render
// lock and needs the GIL; the fetch renders the input at another frame, which needs that render lock.
TEST(OfxInstanceChangedGIL, FetchWhileAnotherRenderNeedsTheGIL)
{
    if ( !NatronTestPlugin::isAvailable(kGILWaitSource) || !NatronTestPlugin::isAvailable(kGILWaitAnalysis) ) {
        NatronTestPlugin::reportMissing(kGILWaitAnalysis);

        return;
    }
    TestCleanup cleanup("OfxInstanceChangedGIL.FetchWhileAnotherRenderNeedsTheGIL");
    NodePtr source = NatronTestPlugin::createNode(kGILWaitSource);
    cleanup.nodes.push_back(source);
    NodePtr analysis = NatronTestPlugin::createNode(kGILWaitAnalysis);
    cleanup.nodes.push_back(analysis);
    ASSERT_TRUE( source && analysis );
    ASSERT_TRUE( analysis->getApp()->getProject()->connectNodes(0, source, analysis) );
    ASSERT_TRUE( setValueExpression(source) );
    // the fetch must render another frame than the render thread, and both must render
    ASSERT_NE( 50., analysis->getApp()->getTimeLine()->currentFrame() );
    appPTR->clearAllCaches();

    const int sourceRenders = NatronTestPlugin::counter("GILWait.sourceRender");
    const int fetched = NatronTestPlugin::counter("GILWait.fetched");
    const int fetchFailed = NatronTestPlugin::counter("GILWait.fetchFailed");
    NatronTestPlugin::setCounter("GILWait.go", 0);
    NatronTestPlugin::setCounter("GILWait.inRender", 0);
    NatronTestPlugin::setCounter("GILWait.holdRender", 1);
    {
        Watchdog watchdog("OfxInstanceChangedGIL.FetchWhileAnotherRenderNeedsTheGIL");
        // a render of frame 50 on another thread (a viewer or background render): it holds the render lock
        // of the source (an InstanceSafe plug-in) until it has read "value", which needs the GIL
        std::atomic<bool> rendered(false);
        std::unique_ptr<JoiningThread> renderThread( new JoiningThread([&source, &rendered]() {
            rendered = bool( NatronTestPlugin::renderFrame(source, 50.) );
            // as Natron's render threads do: no thread-local data stays keyed by this thread once it has exited
            appPTR->getAppTLS()->cleanupTLSForThread();
        }) );
        for (int i = 0; i < 2000 && NatronTestPlugin::counter("GILWait.inRender") == 0; ++i) {
            std::this_thread::sleep_for( std::chrono::milliseconds(10) );
        }
        EXPECT_EQ( 1, NatronTestPlugin::counter("GILWait.inRender") ) << "the render on the other thread did not start";
        {
            PythonGILLocker pgl; // the script runs
            NatronTestPlugin::setCounter("GILWait.go", 1); // the render now waits for the GIL
            // kOfxActionInstanceChanged fetches the source at the current frame (not 50)
            pressButtonFromPython(analysis, "fetch");
        }
        renderThread.reset(); // join
        EXPECT_TRUE(rendered);
    }
    EXPECT_EQ( fetched + 1, NatronTestPlugin::counter("GILWait.fetched") );
    EXPECT_EQ( fetchFailed, NatronTestPlugin::counter("GILWait.fetchFailed") );
    EXPECT_EQ( sourceRenders + 2, NatronTestPlugin::counter("GILWait.sourceRender") );
}

// The script presses "threads": kOfxActionInstanceChanged waits for threads of the multi-thread suite that
// read a parameter with a Python expression.
TEST(OfxInstanceChangedGIL, MultiThreadSuiteThreadsNeedTheGIL)
{
    if ( !NatronTestPlugin::isAvailable(kGILWaitAnalysis) ) {
        NatronTestPlugin::reportMissing(kGILWaitAnalysis);

        return;
    }
    TestCleanup cleanup("OfxInstanceChangedGIL.MultiThreadSuiteThreadsNeedTheGIL");
    NodePtr analysis = NatronTestPlugin::createNode(kGILWaitAnalysis);
    cleanup.nodes.push_back(analysis);
    ASSERT_TRUE( bool(analysis) );
    ASSERT_TRUE( setValueExpression(analysis) );

    const int threadReads = NatronTestPlugin::counter("GILWait.threadReads");
    const int threadReadsFailed = NatronTestPlugin::counter("GILWait.threadReadsFailed");
    const int onOtherThread = NatronTestPlugin::counter("GILWait.threadReadsOnOtherThread");
    {
        Watchdog watchdog("OfxInstanceChangedGIL.MultiThreadSuiteThreadsNeedTheGIL");
        pressButtonFromPython(analysis, "threads");
    }
    if ( NatronTestPlugin::counter("GILWait.threadReadsOnOtherThread") == onOtherThread ) {
        // multiThread() ran the thread functions in the calling thread (one CPU, multithreading off in the
        // preferences, or QFuture::waitForFinished() ran them): nothing waited for another thread
        NatronTest::skipPart("the wait for the GIL in another thread: the thread functions ran in the calling thread (" +
                             std::to_string( NatronTestPlugin::counter("GILWait.numCPUs") ) + " CPUs)");
    }
    EXPECT_EQ( threadReads + 2, NatronTestPlugin::counter("GILWait.threadReads") );
    EXPECT_EQ( threadReadsFailed, NatronTestPlugin::counter("GILWait.threadReadsFailed") );
}
