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

// Natron renders an Unsafe plug-in while holding the node's render lock and the plug-in's render lock, shared by all
// its instances, and an Instance-safe one while holding the node's render lock. When the render action fetches an
// image that was not rendered before the action (a frame not asked for in getFramesNeeded), Natron renders it in
// clipGetImage, or waits for the thread that renders it. It did so with the plug-in's lock still held, so that two
// renders could deadlock (#1614). The test plug-ins (Tests/OfxTestPlugin/NatronTestPluginRenderLock.cpp) wait in
// their action, so that the renders always meet that way.

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <string>
#include <thread>

#include "Engine/AppManager.h"
#include "Engine/KnobTypes.h"
#include "Engine/Node.h"
#include "Engine/Project.h"

#include "OfxTestPlugin.h"

#define kNatronTestPluginRenderLockUnsafe "net.natron.test.RenderLockUnsafe"
#define kNatronTestPluginRenderLockInstanceSafe "net.natron.test.RenderLockInstanceSafe"
#define kNatronTestPluginRenderLockUnsafeGL "net.natron.test.RenderLockUnsafeGL"

NATRON_NAMESPACE_USING

namespace {
const std::chrono::seconds kDeadlockTime(60);

bool
connect(const NodePtr& input,
        const NodePtr& output)
{
    return input && output && output->getApp()->getProject()->connectNodes(0, input, output);
}

// Sets the "fetchTime" parameter of a test plug-in node: its render action at that time fetches another frame
bool
setFetchTime(const NodePtr& node,
             double time)
{
    KnobDoublePtr knob = std::dynamic_pointer_cast<KnobDouble>( node->getKnobByName("fetchTime") );

    if (!knob) {
        return false;
    }
    knob->setValue(time);

    return true;
}

// Sets the "waitFor" parameter of a test plug-in node: the label of the plug-in whose "inRender" counter its fetching
// render action waits for, or "-" for no wait
bool
setWaitFor(const NodePtr& node,
           const char* label)
{
    KnobStringPtr knob = std::dynamic_pointer_cast<KnobString>( node->getKnobByName("waitFor") );

    if (!knob) {
        return false;
    }
    knob->setValue(label);

    return true;
}

// Wait (5 s at most) until the counter of the test plug-in is set
bool
waitForCounter(const char* name)
{
    const std::chrono::steady_clock::time_point deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);

    while ( (NatronTestPlugin::counter(name) == 0) && (std::chrono::steady_clock::now() < deadline) ) {
        std::this_thread::sleep_for( std::chrono::milliseconds(1) );
    }

    return NatronTestPlugin::counter(name) != 0;
}

bool
requireRenderLockPlugins()
{
    for (const char* id : { kNatronTestPluginGenerator, kNatronTestPluginRenderLockUnsafe, kNatronTestPluginRenderLockInstanceSafe,
                            kNatronTestPluginRenderLockUnsafeGL }) {
        if ( !NatronTestPlugin::isAvailable(id) ) {
            NatronTestPlugin::reportMissing(id);

            return false;
        }
    }
    for (const char* label : { "Unsafe", "InstanceSafe", "UnsafeGL", "Test" }) {
        for (const char* counter : { ".inRender", ".waitTimeout", ".fetched", ".renderedByFetcher", ".renderedElsewhere", ".overlap", ".concurrent",
                                     ".glRenders", ".textureErrMemory" }) {
            NatronTestPlugin::setCounter(std::string("RenderLock.") + label + counter, 0);
        }
    }

    return true;
}

// Renders frame of output on a thread (with OpenGL if it is enabled, unless allowGPURendering is false); finished is
// incremented when the render returns
class RenderThread
{
public:
    RenderThread(const NodePtr& output,
                 double frame,
                 std::atomic<int>& finished,
                 bool allowGPURendering = true)
        : _rendered(false)
        , _thread([this, output, frame, &finished, allowGPURendering]() {
        _rendered = bool( NatronTestPlugin::renderFrame(output, frame, allowGPURendering) );
        ++finished;
    })
    {
    }

    RenderThread(const RenderThread&) = delete;
    RenderThread& operator=(const RenderThread&) = delete;

    // true if the render worked; call once, after finished counts this thread
    bool join()
    {
        _thread.join();

        return _rendered;
    }

private:
    bool _rendered;
    std::thread _thread;
};

// Wait until count renders have finished. Renders that deadlock can't be stopped, and the project can't be reset
// while they render: the test binary then exits.
void
waitOrExit(const std::atomic<int>& finished,
           int count)
{
    const std::chrono::steady_clock::time_point deadline = std::chrono::steady_clock::now() + kDeadlockTime;

    while ( (finished < count) && (std::chrono::steady_clock::now() < deadline) ) {
        std::this_thread::sleep_for( std::chrono::milliseconds(1) );
    }
    if (finished < count) {
        ADD_FAILURE() << "deadlock: the renders did not finish in " << kDeadlockTime.count() << " s (#1614); exiting";
        std::cout.flush();
        std::cerr.flush();
        std::fflush(0);
        std::_Exit(EXIT_FAILURE);
    }
}
} // anonymous namespace

// Generator -> U1 (Unsafe plug-in P) -> S (Instance-safe) -> U2 (P), with two frames rendered at the same time:
// - the render of frame 2 holds the lock of S, and the action of S fetches a frame of U1, which needs P's lock;
// - the render of frame 1 holds P's lock, and the action of U2 fetches a frame of S, which needs the lock of S.
TEST(OfxRenderLock, LockOfInstanceSafeNode)
{
    if ( !requireRenderLockPlugins() ) {
        return;
    }
    NatronTest::NodeScope nodes;
    NodePtr generator = nodes.create(kNatronTestPluginGenerator);
    NodePtr u1 = nodes.create(kNatronTestPluginRenderLockUnsafe);
    NodePtr s = nodes.create(kNatronTestPluginRenderLockInstanceSafe);
    NodePtr u2 = nodes.create(kNatronTestPluginRenderLockUnsafe);
    ASSERT_TRUE( connect(generator, u1) );
    ASSERT_TRUE( connect(u1, s) );
    ASSERT_TRUE( connect(s, u2) );
    ASSERT_TRUE( setFetchTime(s, 2.) );
    ASSERT_TRUE( setFetchTime(u2, 1.) );

    // The images that the render of frame 2 (of U1) and the render of frame 1 (of S) need before they take the
    // lock of S and P's lock, already in the cache: the renders get them without waiting for the lock that the
    // other one holds
    ASSERT_TRUE( bool( NatronTestPlugin::renderFrame(u1, 2.) ) );
    ASSERT_TRUE( bool( NatronTestPlugin::renderFrame(s, 1.) ) );

    std::atomic<int> finished(0);
    // frame 2: holds the lock of S in the action of S, until the render of frame 1 is in the action of U2
    RenderThread render2(u2, 2., finished);
    EXPECT_TRUE( waitForCounter("RenderLock.InstanceSafe.inRender") );
    // frame 1: holds P's lock in the action of U2
    RenderThread render1(u2, 1., finished);
    waitOrExit(finished, 2);
    EXPECT_TRUE( render1.join() );
    EXPECT_TRUE( render2.join() );
    // the renders met in the actions, and the images rendered in clipGetImage were fetched
    EXPECT_EQ( 0, NatronTestPlugin::counter("RenderLock.Unsafe.waitTimeout") );
    EXPECT_EQ( 0, NatronTestPlugin::counter("RenderLock.InstanceSafe.waitTimeout") );
    EXPECT_EQ( 1, NatronTestPlugin::counter("RenderLock.Unsafe.fetched") );
    EXPECT_EQ( 1, NatronTestPlugin::counter("RenderLock.InstanceSafe.fetched") );
    // an instance never rendered twice at the same time (U2 renders on its main instance, while S has render clones)
    EXPECT_EQ( 0, NatronTestPlugin::counter("RenderLock.Unsafe.overlap") );
    EXPECT_EQ( 0, NatronTestPlugin::counter("RenderLock.InstanceSafe.overlap") );
    // the code of the Unsafe plug-in never ran on two threads at the same time
    EXPECT_EQ( 0, NatronTestPlugin::counter("RenderLock.Unsafe.concurrent") );
}

// Generator -> U1 (Unsafe plug-in P) -> U2 (P), no other render lock:
// - the render of frame f holds P's lock in the action of U2, which fetches the frame f + 100 of U1;
// - the render of frame f + 100 of U1, on another thread, has marked the pixels of that image as being rendered by
//   it, and waits for P's lock.
// The render of frame f waited for these pixels with P's lock held.
// Nothing tells when the second render has marked its pixels (for an image that isn't cached yet, it marks them
// after it has rendered its inputs, and then only waits for locks): the test gives it some time, and tries again
// with other frames and more time if the first render rendered them itself instead.
TEST(OfxRenderLock, PixelsRenderedElsewhere)
{
    if ( !requireRenderLockPlugins() ) {
        return;
    }
    NatronTest::NodeScope nodes;
    NodePtr generator = nodes.create(kNatronTestPluginGenerator);
    NodePtr u1 = nodes.create(kNatronTestPluginRenderLockUnsafe);
    NodePtr u2 = nodes.create(kNatronTestPluginRenderLockUnsafe);
    ASSERT_TRUE( connect(generator, u1) );
    ASSERT_TRUE( connect(u1, u2) );

    bool metElsewhere = false;
    // 250 ms to 8 s: less than the 10 s that the action of U2 waits for the test
    std::chrono::milliseconds delay(250);
    for (int attempt = 0; attempt < 6 && !metElsewhere; ++attempt, delay *= 2) {
        const double frame = 1. + attempt;
        ASSERT_TRUE( setFetchTime(u2, frame) );
        for (const char* counter : { "Unsafe.inRender", "InstanceSafe.inRender", "Unsafe.fetched", "Unsafe.renderedElsewhere", "Unsafe.renderedByFetcher" }) {
            NatronTestPlugin::setCounter(std::string("RenderLock.") + counter, 0);
        }
        std::atomic<int> finished(0);
        // frame f: holds P's lock in the action of U2, which waits until the test sets "RenderLock.InstanceSafe.inRender"
        RenderThread renderF(u2, frame, finished);
        EXPECT_TRUE( waitForCounter("RenderLock.Unsafe.inRender") );
        // frame f + 100 of U1: marks its pixels, then waits for P's lock
        RenderThread renderF100(u1, frame + 100., finished);
        std::this_thread::sleep_for(delay);
        NatronTestPlugin::setCounter("RenderLock.InstanceSafe.inRender", 1);
        waitOrExit(finished, 2);
        EXPECT_TRUE( renderF.join() );
        EXPECT_TRUE( renderF100.join() );
        // the scenario: frame f + 100 was rendered by its own render only, not (also) in the clipGetImage of U2,
        // which got it
        metElsewhere = NatronTestPlugin::counter("RenderLock.Unsafe.renderedElsewhere") == 1 &&
                       NatronTestPlugin::counter("RenderLock.Unsafe.renderedByFetcher") == 0 &&
                       NatronTestPlugin::counter("RenderLock.Unsafe.fetched") == 1;
    }
    EXPECT_TRUE(metElsewhere) << "the render of the fetched frame never marked its pixels before it was fetched";
    EXPECT_EQ( 0, NatronTestPlugin::counter("RenderLock.Unsafe.waitTimeout") );
    EXPECT_EQ( 0, NatronTestPlugin::counter("RenderLock.Unsafe.overlap") );
    EXPECT_EQ( 0, NatronTestPlugin::counter("RenderLock.Unsafe.concurrent") );
}

// The same deadlocks with an OpenGL render of an Unsafe plug-in, which holds its plug-in lock and the OpenGL context
// during the action (#1998): the renders upstream of the action must not wait. Each test renders the frame on the
// CPU after its OpenGL render could not fetch its input (kOfxStatErrMemory), as when an input texture can't be
// allocated (#1945). OpenGL rendering needs a renderer: on Linux, the tests run with Mesa's llvmpipe under Xvfb in
// the TestsOpenGLSoftware test (Tests/CMakeLists.txt), and are skipped in the Tests test, where it isn't used.
namespace {
bool
requireOpenGLRenderLock(NatronTest::ScopedOpenGLRendering& enabler)
{
    if ( !requireRenderLockPlugins() ) {
        return false;
    }
    // TestsOpenGLSoftware exists to run these tests: there, no OpenGL rendering (e.g. no X display) is a failure
    const char* softwareRenderer = std::getenv("NATRON_OPENGL_ALLOW_SOFTWARE_RENDERER");
    if ( softwareRenderer && (std::string(softwareRenderer) == "1") &&
         ( !appPTR->isOpenGLLoaded() || appPTR->getOpenGLRenderers().empty() ) ) {
        ADD_FAILURE() << "NATRON_OPENGL_ALLOW_SOFTWARE_RENDERER is set, but OpenGL rendering can't be used (is there an X display?)";

        return false;
    }
    if ( !NatronTest::requireOpenGLRendering() ) {
        return false;
    }
    if ( !enabler.isEnabled() ) {
        ADD_FAILURE() << "OpenGL rendering could not be enabled";

        return false;
    }

    return true;
}
} // anonymous namespace

// Generator -> X (Instance-safe) -> B (Unsafe plug-in P, OpenGL) -> U2 (P, OpenGL):
// - the render of frame 2 of B, on the CPU, holds the node lock of B, and B's action fetches the frame 102 of X with
//   P's lock released; the action of X waits until the OpenGL render of U2 could not fetch B(101);
// - the OpenGL render of frame 1 of U2 holds P's lock and the OpenGL context, and fetches the frame 101 of B, which
//   needs the node lock of B, while the render of B waits for P's lock to go on.
TEST(OfxRenderLockGPU, NodeLockOfAnotherNode)
{
    NatronTest::ScopedOpenGLRendering enabler;
    if ( !requireOpenGLRenderLock(enabler) ) {
        return;
    }
    NatronTest::NodeScope nodes;
    NodePtr generator = nodes.create(kNatronTestPluginGenerator);
    NodePtr x = nodes.create(kNatronTestPluginRenderLockInstanceSafe);
    NodePtr b = nodes.create(kNatronTestPluginRenderLockUnsafeGL);
    NodePtr u2 = nodes.create(kNatronTestPluginRenderLockUnsafeGL);
    ASSERT_TRUE( connect(generator, x) );
    ASSERT_TRUE( connect(x, b) );
    ASSERT_TRUE( connect(b, u2) );
    ASSERT_EQ( ePluginOpenGLRenderSupportYes, u2->getCurrentOpenGLRenderSupport() );
    ASSERT_TRUE( setFetchTime(x, 102.) );
    // until the OpenGL render of U2 could not fetch B(101) (the test sets "RenderLock.Test.inRender" then)
    ASSERT_TRUE( setWaitFor(x, "Test") );
    ASSERT_TRUE( setFetchTime(b, 2.) );
    ASSERT_TRUE( setWaitFor(b, "-") );
    ASSERT_TRUE( setFetchTime(u2, 1.) );

    std::atomic<int> finished(0);
    // frame 2 of B, on the CPU: holds the node lock of B, and waits in the action of X
    RenderThread render2(b, 2., finished, false /*allowGPURendering*/);
    EXPECT_TRUE( waitForCounter("RenderLock.InstanceSafe.inRender") );
    // frame 1 of U2, with OpenGL
    RenderThread render1(u2, 1., finished);
    // its fetch of B(101) failed: let the render of frame 2 go on
    EXPECT_TRUE( waitForCounter("RenderLock.UnsafeGL.textureErrMemory") );
    NatronTestPlugin::setCounter("RenderLock.Test.inRender", 1);
    waitOrExit(finished, 2);
    EXPECT_TRUE( render1.join() );
    EXPECT_TRUE( render2.join() );
    EXPECT_EQ( 0, NatronTestPlugin::counter("RenderLock.InstanceSafe.waitTimeout") );
    EXPECT_EQ( 0, NatronTestPlugin::counter("RenderLock.UnsafeGL.waitTimeout") );
    // the OpenGL render of U2 ran, could not fetch B(101), and U2 was rendered again on the CPU, which fetched it
    EXPECT_LE( 1, NatronTestPlugin::counter("RenderLock.UnsafeGL.glRenders") );
    EXPECT_EQ( 1, NatronTestPlugin::counter("RenderLock.UnsafeGL.textureErrMemory") );
    // X(102) fetched by B, and B(101) fetched by the CPU render of U2
    EXPECT_EQ( 2, NatronTestPlugin::counter("RenderLock.UnsafeGL.fetched") );
    EXPECT_EQ( 1, NatronTestPlugin::counter("RenderLock.InstanceSafe.fetched") );
    EXPECT_EQ( 0, NatronTestPlugin::counter("RenderLock.UnsafeGL.overlap") );
    EXPECT_EQ( 0, NatronTestPlugin::counter("RenderLock.UnsafeGL.concurrent") );
}

// Generator -> U1 (Unsafe plug-in P, OpenGL) -> U2 (P, OpenGL), as PixelsRenderedElsewhere: the OpenGL render of
// frame f of U2 holds P's lock and the OpenGL context, and fetches the frame f + 100 of U1, whose pixels the render
// of that frame (on the CPU, on another thread) has marked before it waits for P's lock. Before #1998 the OpenGL
// render waited for these pixels with P's lock held.
TEST(OfxRenderLockGPU, PixelsRenderedElsewhere)
{
    NatronTest::ScopedOpenGLRendering enabler;
    if ( !requireOpenGLRenderLock(enabler) ) {
        return;
    }
    NatronTest::NodeScope nodes;
    NodePtr generator = nodes.create(kNatronTestPluginGenerator);
    NodePtr u1 = nodes.create(kNatronTestPluginRenderLockUnsafeGL);
    NodePtr u2 = nodes.create(kNatronTestPluginRenderLockUnsafeGL);
    ASSERT_TRUE( connect(generator, u1) );
    ASSERT_TRUE( connect(u1, u2) );
    ASSERT_EQ( ePluginOpenGLRenderSupportYes, u2->getCurrentOpenGLRenderSupport() );

    bool metElsewhere = false;
    // 250 ms to 8 s: less than the 10 s that the action of U2 waits for the test
    std::chrono::milliseconds delay(250);
    for (int attempt = 0; attempt < 6 && !metElsewhere; ++attempt, delay *= 2) {
        const double frame = 1. + attempt;
        ASSERT_TRUE( setFetchTime(u2, frame) );
        for (const char* counter : { "UnsafeGL.inRender", "InstanceSafe.inRender", "UnsafeGL.fetched", "UnsafeGL.textureErrMemory",
                                     "UnsafeGL.renderedElsewhere", "UnsafeGL.renderedByFetcher" }) {
            NatronTestPlugin::setCounter(std::string("RenderLock.") + counter, 0);
        }
        std::atomic<int> finished(0);
        // frame f, with OpenGL: holds P's lock in the action of U2, which waits until the test sets
        // "RenderLock.InstanceSafe.inRender"
        RenderThread renderF(u2, frame, finished);
        EXPECT_TRUE( waitForCounter("RenderLock.UnsafeGL.inRender") );
        // frame f + 100 of U1, on the CPU: marks its pixels, then waits for P's lock
        RenderThread renderF100(u1, frame + 100., finished, false /*allowGPURendering*/);
        std::this_thread::sleep_for(delay);
        NatronTestPlugin::setCounter("RenderLock.InstanceSafe.inRender", 1);
        waitOrExit(finished, 2);
        EXPECT_TRUE( renderF.join() );
        EXPECT_TRUE( renderF100.join() );
        // the scenario: the OpenGL render of U2 could not fetch the frame f + 100 (being rendered by the other
        // thread), and the CPU render of U2 fetched it once the other thread had rendered it
        metElsewhere = NatronTestPlugin::counter("RenderLock.UnsafeGL.renderedElsewhere") == 1 &&
                       NatronTestPlugin::counter("RenderLock.UnsafeGL.renderedByFetcher") == 0 &&
                       NatronTestPlugin::counter("RenderLock.UnsafeGL.textureErrMemory") == 1 &&
                       NatronTestPlugin::counter("RenderLock.UnsafeGL.fetched") == 1;
    }
    EXPECT_TRUE(metElsewhere) << "the render of the fetched frame never marked its pixels before it was fetched";
    EXPECT_LE( 1, NatronTestPlugin::counter("RenderLock.UnsafeGL.glRenders") );
    EXPECT_EQ( 0, NatronTestPlugin::counter("RenderLock.UnsafeGL.waitTimeout") );
    EXPECT_EQ( 0, NatronTestPlugin::counter("RenderLock.UnsafeGL.overlap") );
    EXPECT_EQ( 0, NatronTestPlugin::counter("RenderLock.UnsafeGL.concurrent") );
}

// Generator -> X (Instance-safe) -> N (Unsafe plug-in P, OpenGL) -> U2 (P, OpenGL): the OpenGL action of U2 fetches
// the frame 101 of N, rendered on the CPU (another time), whose own action fetches the frame 201 of X while another
// thread holds the node lock of X (in the action of X at frame 202, which waits until that fetch failed).
// N asks for no region of its input (see NatronTestPluginRenderLock.cpp), so only its action fetches X. The render
// of N(101) must not wait (U2 holds the OpenGL context), nor be cached without its input, nor set a persistent
// message on N: U2 renders the frame again on the CPU, which renders N(101) again once X is free.
TEST(OfxRenderLockGPU, RefusedWaitInAnUpstreamAction)
{
    NatronTest::ScopedOpenGLRendering enabler;
    if ( !requireOpenGLRenderLock(enabler) ) {
        return;
    }
    NatronTest::NodeScope nodes;
    NodePtr generator = nodes.create(kNatronTestPluginGenerator);
    NodePtr x = nodes.create(kNatronTestPluginRenderLockInstanceSafe);
    NodePtr n = nodes.create(kNatronTestPluginRenderLockUnsafeGL);
    NodePtr u2 = nodes.create(kNatronTestPluginRenderLockUnsafeGL);
    ASSERT_TRUE( connect(generator, x) );
    ASSERT_TRUE( connect(x, n) );
    ASSERT_TRUE( connect(n, u2) );
    ASSERT_EQ( ePluginOpenGLRenderSupportYes, u2->getCurrentOpenGLRenderSupport() );
    ASSERT_TRUE( setFetchTime(x, 202.) );
    // until the OpenGL render of U2 could not fetch N(101) (the test sets "RenderLock.Test.inRender" then)
    ASSERT_TRUE( setWaitFor(x, "Test") );
    ASSERT_TRUE( setFetchTime(n, 101.) );
    ASSERT_TRUE( setWaitFor(n, "-") );
    ASSERT_TRUE( setFetchTime(u2, 1.) );

    std::atomic<int> finished(0);
    // frame 202 of X: holds the node lock of X in its action, until the OpenGL action of U2 runs
    RenderThread render202(x, 202., finished, false /*allowGPURendering*/);
    EXPECT_TRUE( waitForCounter("RenderLock.InstanceSafe.inRender") );
    // frame 1 of U2, with OpenGL
    RenderThread render1(u2, 1., finished);
    // its fetch of N(101) failed: let the render of frame 202 go on
    EXPECT_TRUE( waitForCounter("RenderLock.UnsafeGL.textureErrMemory") );
    NatronTestPlugin::setCounter("RenderLock.Test.inRender", 1);
    waitOrExit(finished, 2);
    EXPECT_TRUE( render1.join() );
    EXPECT_TRUE( render202.join() );
    EXPECT_EQ( 0, NatronTestPlugin::counter("RenderLock.InstanceSafe.waitTimeout") );
    EXPECT_EQ( 0, NatronTestPlugin::counter("RenderLock.UnsafeGL.waitTimeout") );
    // the OpenGL render of U2 could not fetch N(101), and U2 was rendered again on the CPU, which fetched it
    EXPECT_LE( 1, NatronTestPlugin::counter("RenderLock.UnsafeGL.glRenders") );
    EXPECT_EQ( 1, NatronTestPlugin::counter("RenderLock.UnsafeGL.textureErrMemory") );
    // N(101) was rendered again with its input X(201), not taken from the cache without it: the fetch of X(201) by
    // N, and the fetch of N(101) by the CPU render of U2
    EXPECT_EQ( 2, NatronTestPlugin::counter("RenderLock.UnsafeGL.fetched") );
    // the refused fetch is not a failure of N
    EXPECT_FALSE( n->hasPersistentMessage() );
    EXPECT_EQ( 0, NatronTestPlugin::counter("RenderLock.UnsafeGL.overlap") );
    EXPECT_EQ( 0, NatronTestPlugin::counter("RenderLock.UnsafeGL.concurrent") );
}
