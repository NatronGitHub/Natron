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

// A render recurses through every node upstream of the rendered node (renderRoI,
// renderInputImagesForRoI, getRegionOfDefinition...), on a render thread or a thread of the global
// thread pool. With the default stack size of secondary threads (512 KB on macOS), a linear chain of
// about 300 nodes overflowed the stack and crashed (#1434). The render threads and the global thread
// pool now have a stack of getRenderThreadStackSize() bytes (at least NATRON_RENDER_THREAD_STACK_SIZE).

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <algorithm>
#include <chrono>
#include <future>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

#include <QtCore/QTemporaryDir>
#include <QThread>
#include <QThreadPool>

#include "Engine/GraphDepthGuard.h"
#include "Engine/Node.h"
#include "Engine/Project.h"
#include "Engine/ThreadPool.h"

#include "OfxTestPlugin.h"

NATRON_NAMESPACE_USING

namespace {
// Number of filters in the chain: with 512 KB stacks, the render of this chain overflowed the stack
// of the render thread at a depth of about 440 nodes (in the getRegionOfDefinition recursion).
const int kChainLength = 600;

// Number of filters of a chain that is too deep for the stacks of Natron's threads (#1440): with
// 8 MB stacks, a render of about 3500 MultiPAR nodes overflowed it, and with GraphDepthGuard the
// limit is between 3000 and 3200 (macOS arm64). Such a chain must either render, or fail with the
// error "Node graph too deep", but never crash.
const int kVeryDeepChainLength = 5000;

// Create a node with a unique script name: finding a free default name ("MultiPAR1", "MultiPAR2"...)
// tries every number in turn, and each try scans every node, so that building a chain of thousands of
// nodes with default names takes minutes.
NodePtr
createChainNode(const char* pluginID)
{
    static int count = 0;
    AppInstancePtr app = appPTR->getTopLevelInstance();
    CreateNodeArgs args( pluginID, app->getProject() );

    args.setProperty<bool>(kCreateNodeArgsPropAddUndoRedoCommand, false);
    args.setProperty<bool>(kCreateNodeArgsPropAutoConnect, false);
    args.setProperty<bool>(kCreateNodeArgsPropSettingsOpened, false);
    args.setProperty<std::string>( kCreateNodeArgsPropNodeInitialName, "DeepChain" + std::to_string(++count) + "_" );

    return app->createNode(args);
}

// net.natron.test.Generator -> length x filter (net.natron.test.MultiPAR, or another plug-in) -> DiskCache
struct DeepChain
{
    std::vector<NodePtr> nodes; // from the generator to the output
    int length;

    DeepChain(int length = kChainLength,
              const char* filterID = kNatronTestPluginMultiPAR)
        : nodes()
        , length(length)
    {
        NodePtr generator = createChainNode(kNatronTestPluginGenerator);
        if (!generator) {
            return;
        }
        nodes.push_back(generator);
        ProjectPtr project = generator->getApp()->getProject();
        for (int i = 0; i < length; ++i) {
            NodePtr filter = createChainNode(filterID);
            if ( !filter || !project->connectNodes(0, nodes.back(), filter) ) {
                return;
            }
            nodes.push_back(filter);
        }
        NodePtr output = createChainNode(PLUGINID_NATRON_DISKCACHE);
        if ( !output || !project->connectNodes(0, nodes.back(), output) ) {
            return;
        }
        nodes.push_back(output);
    }

    ~DeepChain()
    {
        for (std::vector<NodePtr>::reverse_iterator it = nodes.rbegin(); it != nodes.rend(); ++it) {
            (*it)->destroyNode(true, false);
        }
    }

    bool isValid() const
    {
        return (int)nodes.size() == length + 2;
    }

    const NodePtr& output() const
    {
        return nodes.back();
    }

    // the last filter
    const NodePtr& lastFilter() const
    {
        return nodes[nodes.size() - 2];
    }
};

// Sets the maximum depth of GraphDepthGuard, and restores it even if an ASSERT fails
struct MaxGraphDepthSetter
{
    int saved;

    MaxGraphDepthSetter(int maxDepth)
        : saved( GraphDepthGuard::getMaxDepth() )
    {
        GraphDepthGuard::setMaxDepth(maxDepth);
    }

    ~MaxGraphDepthSetter()
    {
        GraphDepthGuard::setMaxDepth(saved);
    }
};

// Prints the time since the previous call on stderr (stdout is captured by some tests)
void
printStep(const char* step)
{
    static std::chrono::steady_clock::time_point last = std::chrono::steady_clock::now();
    const std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
    std::cerr << "DeepGraphRender: " << step << " ("
              << std::chrono::duration_cast<std::chrono::milliseconds>(now - last).count() << " ms)" << std::endl;
    last = now;
}

// The error reported by GraphDepthGuard. In background mode, persistent messages are printed.
bool
isGraphTooDeepError(const std::string& output)
{
    return output.find("Node graph too deep") != std::string::npos;
}

// Render the frame of node on a thread of the global thread pool, as the viewer does. Returns
// (rendered, ran on a pool thread).
std::pair<bool, bool>
renderFrameOnGlobalThreadPool(const NodePtr& node)
{
    // Not QtConcurrent::run(): QFuture::result() and waitForFinished() run the task on the calling
    // thread (the main thread, with the stack set by the linker) if no pool thread took it yet.
    std::promise<std::pair<bool, bool> > promise; // rendered, ran on a pool thread
    std::future<std::pair<bool, bool> > rendered = promise.get_future();
    QThread* const testThread = QThread::currentThread();
    QThreadPool::globalInstance()->start( [node, testThread, &promise]() {
        const bool onPoolThread = QThread::currentThread() != testThread;
        promise.set_value( std::make_pair(bool( NatronTestPlugin::renderFrame(node, 1.) ), onPoolThread) );
    } );

    return rendered.get();
}

bool
canRunTest()
{
    if ( !NatronTestPlugin::isAvailable(kNatronTestPluginGenerator) ) {
        NatronTestPlugin::reportSkipped(kNatronTestPluginGenerator);

        return false;
    }
    if ( !NatronTestPlugin::isAvailable(kNatronTestPluginMultiPAR) ) {
        NatronTestPlugin::reportSkipped(kNatronTestPluginMultiPAR);

        return false;
    }

    return true;
}
} // anon namespace

// At least NATRON_RENDER_THREAD_STACK_SIZE, and never less than the default stack size of new threads
TEST(DeepGraphRender, StackSize)
{
    EXPECT_GE( getRenderThreadStackSize(), (unsigned int)NATRON_RENDER_THREAD_STACK_SIZE );
}

// The stack size applies to the threads that the global pool creates after AppManager::load().
TEST(DeepGraphRender, GlobalThreadPoolStackSize)
{
    EXPECT_EQ( getRenderThreadStackSize(), QThreadPool::globalInstance()->stackSize() );
}

namespace {
// Derives from AbortableThread like RenderThreadTask, GenericSchedulerThread and OfxThread
class TestAbortableThread
    : public QThread
    , public AbortableThread
{
public:
    TestAbortableThread()
        : QThread()
        , AbortableThread(this)
    {
    }

private:
    void run() OVERRIDE {}
};
} // anon namespace

// Every thread that renders derives from AbortableThread, which sets its stack size.
TEST(DeepGraphRender, AbortableThreadStackSize)
{
    TestAbortableThread thread;

    EXPECT_EQ( getRenderThreadStackSize(), thread.stackSize() );
}

// Render a frame of the chain on disk: on a render thread (RenderThreadTask), as NatronRenderer does.
TEST(DeepGraphRender, RenderThread)
{
    if ( !canRunTest() ) {
        return;
    }
    appPTR->clearAllCaches();
    DeepChain chain;
    ASSERT_TRUE( chain.isValid() );

    const int rendersBefore = NatronTestPlugin::counter("MultiPAR.render");
    NatronTestPlugin::renderSequence(chain.output(), 1, 1);
    EXPECT_EQ( kChainLength, NatronTestPlugin::counter("MultiPAR.render") - rendersBefore );
}

// Render the chain on a thread of the global thread pool, as the viewer does.
TEST(DeepGraphRender, GlobalThreadPool)
{
    if ( !canRunTest() ) {
        return;
    }
    appPTR->clearAllCaches();
    DeepChain chain;
    ASSERT_TRUE( chain.isValid() );

    const int rendersBefore = NatronTestPlugin::counter("MultiPAR.render");
    const std::pair<bool, bool> result = renderFrameOnGlobalThreadPool( chain.lastFilter() );
    EXPECT_TRUE(result.second) << "the render must run on a thread of the global thread pool";
    EXPECT_TRUE(result.first);
    EXPECT_EQ( kChainLength, NatronTestPlugin::counter("MultiPAR.render") - rendersBefore );
}

namespace {
// Runs on an AbortableThread, like the render threads, and gets GraphDepthGuard::getStackLeft() there
class StackLeftThread
    : public QThread
    , public AbortableThread
{
public:
    std::size_t stackLeft;

    StackLeftThread()
        : QThread()
        , AbortableThread(this)
        , stackLeft(0)
    {
    }

private:
    void run() OVERRIDE
    {
        stackLeft = GraphDepthGuard::getStackLeft();
    }
};
} // anon namespace

// GraphDepthGuard knows the stack bounds of the main thread, of the render threads and of the threads
// of the global thread pool, on every platform: otherwise it falls back to a fixed maximum depth.
TEST(DeepGraphRender, StackBounds)
{
    const std::size_t MB = 1024 * 1024;
    // 8 MB on macOS, on Windows (set at link time) and usually on Linux (ulimit -s), minus the
    // safety margin and what is already used
    const std::size_t mainStackLeft = GraphDepthGuard::getStackLeft();
    EXPECT_GT(mainStackLeft, 4 * MB) << "the stack bounds of the main thread are unknown or wrong";

    // most of the stack of a fresh thread is left
#ifdef __NATRON_WIN32__
    // Qt passes the stack size to CreateThread() as the initial commit: the thread reserves the
    // executable's default stack reserve if it is larger, NATRON_MAIN_THREAD_STACK_SIZE (8 MB, see
    // CMakeLists.txt) since #1446.
    const std::size_t renderStackSize = std::max<std::size_t>(getRenderThreadStackSize(), 8 * MB);
#else
    const std::size_t renderStackSize = getRenderThreadStackSize();
#endif
    StackLeftThread thread;
    thread.start();
    thread.wait();
    EXPECT_GT(thread.stackLeft, renderStackSize / 2) << "the stack bounds of a render thread are unknown or wrong";
    EXPECT_LT(thread.stackLeft, renderStackSize);

    std::promise<std::size_t> promise;
    std::future<std::size_t> poolStackLeft = promise.get_future();
    QThreadPool::globalInstance()->start( [&promise]() {
        promise.set_value( GraphDepthGuard::getStackLeft() );
    } );
    const std::size_t poolLeft = poolStackLeft.get();
    EXPECT_GT(poolLeft, renderStackSize / 2) << "the stack bounds of a thread of the global pool are unknown or wrong";
    EXPECT_LT(poolLeft, renderStackSize);
}

// With a maximum depth smaller than the chain, the renders fail with the "Node graph too deep" error
// (#1440), and once the limit is removed the same chain renders: nothing wrong was cached.
TEST(DeepGraphRender, TooDeepIsReported)
{
    if ( !canRunTest() ) {
        return;
    }
    appPTR->clearAllCaches();
    DeepChain chain;
    ASSERT_TRUE( chain.isValid() );

    {
        MaxGraphDepthSetter limit(kChainLength / 2);

        // on the main thread
        testing::internal::CaptureStdout();
        ImagePtr image = NatronTestPlugin::renderFrame(chain.lastFilter(), 1.);
        std::string output = testing::internal::GetCapturedStdout();
        EXPECT_FALSE( bool(image) );
        EXPECT_TRUE( isGraphTooDeepError(output) ) << output;
        // the generator and the filters, upstream of the last filter
        EXPECT_NE( std::string::npos, output.find("has a chain of " + std::to_string(kChainLength + 1) + " nodes") ) << output;
        EXPECT_FALSE( GraphDepthGuard::hasFailed() ) << "the outermost guard must reset the state of the thread";

        // the region of definition
        RectD rod;
        testing::internal::CaptureStdout();
        StatusEnum stat = chain.output()->getEffectInstance()->getRegionOfDefinition_public(chain.output()->getHashValue(), 1., RenderScale::identity, ViewIdx(0), &rod, nullptr);
        output = testing::internal::GetCapturedStdout();
        EXPECT_EQ(eStatusFailed, stat);
        EXPECT_TRUE( isGraphTooDeepError(output) ) << output;

        // on a thread of the global thread pool
        testing::internal::CaptureStdout();
        std::pair<bool, bool> result = renderFrameOnGlobalThreadPool( chain.lastFilter() );
        output = testing::internal::GetCapturedStdout();
        EXPECT_FALSE(result.first);
        EXPECT_TRUE( isGraphTooDeepError(output) ) << output;

        // on a render thread
        const int rendersBefore = NatronTestPlugin::counter("MultiPAR.render");
        testing::internal::CaptureStdout();
        NatronTestPlugin::renderSequence(chain.output(), 1, 1);
        output = testing::internal::GetCapturedStdout();
        EXPECT_EQ( 0, NatronTestPlugin::counter("MultiPAR.render") - rendersBefore );
        EXPECT_TRUE( isGraphTooDeepError(output) ) << output;
    }

    // without the limit, the chain renders on every kind of thread
    int rendersBefore = NatronTestPlugin::counter("MultiPAR.render");
    EXPECT_TRUE( bool( NatronTestPlugin::renderFrame(chain.lastFilter(), 1.) ) );
    EXPECT_EQ( kChainLength, NatronTestPlugin::counter("MultiPAR.render") - rendersBefore );

    appPTR->clearAllCaches();
    rendersBefore = NatronTestPlugin::counter("MultiPAR.render");
    std::pair<bool, bool> result = renderFrameOnGlobalThreadPool( chain.lastFilter() );
    EXPECT_TRUE(result.first);
    EXPECT_EQ( kChainLength, NatronTestPlugin::counter("MultiPAR.render") - rendersBefore );

    appPTR->clearAllCaches();
    rendersBefore = NatronTestPlugin::counter("MultiPAR.render");
    NatronTestPlugin::renderSequence(chain.output(), 1, 1);
    EXPECT_EQ( kChainLength, NatronTestPlugin::counter("MultiPAR.render") - rendersBefore );
}

namespace {
// Each operation on a very deep chain either works or fails with the "Node graph too deep" error.
void
checkVeryDeepChain(const NodePtr& output,
                   const NodePtr& lastFilter,
                   int length)
{
    // the region of definition, on the main thread
    {
        RectD rod;
        testing::internal::CaptureStdout();
        StatusEnum stat = output->getEffectInstance()->getRegionOfDefinition_public(output->getHashValue(), 1., RenderScale::identity, ViewIdx(0), &rod, nullptr);
        std::string out = testing::internal::GetCapturedStdout();
        EXPECT_TRUE( (stat == eStatusOK) || isGraphTooDeepError(out) ) << out;
        printStep(stat == eStatusOK ? "region of definition: OK" : "region of definition: too deep");
    }

    // render on the main thread
    {
        testing::internal::CaptureStdout();
        ImagePtr image = NatronTestPlugin::renderFrame(lastFilter, 1.);
        std::string out = testing::internal::GetCapturedStdout();
        EXPECT_TRUE( image || isGraphTooDeepError(out) ) << out;
        printStep(image ? "render on the main thread: OK" : "render on the main thread: too deep");
    }

    // render on a thread of the global thread pool
    {
        testing::internal::CaptureStdout();
        std::pair<bool, bool> result = renderFrameOnGlobalThreadPool(lastFilter);
        std::string out = testing::internal::GetCapturedStdout();
        EXPECT_TRUE( result.first || isGraphTooDeepError(out) ) << out;
        printStep(result.first ? "render on the global thread pool: OK" : "render on the global thread pool: too deep");
    }

    // render on a render thread, on disk
    {
        appPTR->clearAllCaches();
        const int rendersBefore = NatronTestPlugin::counter("MultiPAR.render");
        testing::internal::CaptureStdout();
        NatronTestPlugin::renderSequence(output, 1, 1);
        std::string out = testing::internal::GetCapturedStdout();
        const int renders = NatronTestPlugin::counter("MultiPAR.render") - rendersBefore;
        EXPECT_TRUE( (renders == length) || isGraphTooDeepError(out) ) << out;
        printStep(isGraphTooDeepError(out) ? "render on a render thread: too deep" : "render on a render thread: OK");
    }
}
} // anon namespace

// A chain of kVeryDeepChainLength nodes, deeper than the stack allows: building it (each connection
// refreshes the hash and the metadata downstream), a change at its top, its region of definition, its
// renders, and saving and loading it work or report the "Node graph too deep" error, but never crash
// (#1440).
TEST(DeepGraphRender, VeryDeepChain)
{
    if ( !canRunTest() ) {
        return;
    }
    appPTR->clearAllCaches();
    QTemporaryDir dir;
    ASSERT_TRUE( dir.isValid() );
    const QString path = dir.path() + QLatin1Char('/');
    const QString name = QString::fromUtf8("deep.ntp");
    ProjectPtr project = appPTR->getTopLevelInstance()->getProject();
    // resets the project even if an ASSERT fails: declared after dir, so the project is reset
    // before the directory it was saved to is removed
    struct Restore
    {
        ProjectPtr project;
        ~Restore()
        {
            project->reset(false, true);
        }
    } restore = { project };
    // start from an empty project: other tests may have left nodes in it
    project->reset(false, true);
    ASSERT_EQ( 0, (int)project->getNodes().size() );
    // Where the stack allows it (Linux CI), the chain renders: with the default HD format, each render
    // of 5000 filters took minutes. The generator's RoD is the project format.
    project->setOrAddProjectFormat( Format(0, 0, 16, 16, "DeepGraphRender", 1.) );
    std::string outputName, lastFilterName;

    printStep("start");
    {
        DeepChain chain(kVeryDeepChainLength);
        ASSERT_TRUE( chain.isValid() );
        printStep("chain built");

        // changing the generator changes the hash of every node downstream
        const U64 hashBefore = chain.output()->getHashValue();
        chain.nodes.front()->incrementKnobsAge();
        EXPECT_NE( hashBefore, chain.output()->getHashValue() );
        printStep("hash changed");

        // reconnecting the generator refreshes the hash, the metadata and the other data of every node
        // downstream, recursively
        ASSERT_TRUE( Project::disconnectNodes(chain.nodes[0], chain.nodes[1]) );
        ASSERT_TRUE( Project::connectNodes(0, chain.nodes[0], chain.nodes[1]) );
        EXPECT_EQ( chain.nodes[0], chain.nodes[1]->getInput(0) );
        printStep("generator reconnected");

        checkVeryDeepChain(chain.output(), chain.lastFilter(), kVeryDeepChainLength);

        outputName = chain.output()->getScriptName();
        lastFilterName = chain.lastFilter()->getScriptName();
        ASSERT_TRUE( project->saveProject(path, name, 0) );
        printStep("project saved");
    }
    printStep("chain destroyed");
    ASSERT_TRUE( project->loadProject(path, name) );
    printStep("project loaded");
    NodePtr output = project->getNodeByName(outputName);
    NodePtr lastFilter = project->getNodeByName(lastFilterName);
    ASSERT_TRUE( bool(output) );
    ASSERT_TRUE( bool(lastFilter) );
    EXPECT_EQ( kVeryDeepChainLength + 2, (int)project->getNodes().size() );

    checkVeryDeepChain(output, lastFilter, kVeryDeepChainLength);
}

// A chain of Dot nodes, which are identities: renderRoI goes straight to the input. Rendering very
// deep Dot chains is slow (a 4000-node chain takes about 40 s), so this uses a shorter chain and a
// lower maximum depth.
TEST(DeepGraphRender, DotChain)
{
    if ( !canRunTest() ) {
        return;
    }
    const int length = 1500;
    appPTR->clearAllCaches();
    DeepChain chain(length, PLUGINID_NATRON_DOT);
    ASSERT_TRUE( chain.isValid() );

    {
        MaxGraphDepthSetter limit(length / 3);

        testing::internal::CaptureStdout();
        std::pair<bool, bool> result = renderFrameOnGlobalThreadPool( chain.lastFilter() );
        std::string output = testing::internal::GetCapturedStdout();
        EXPECT_FALSE(result.first);
        EXPECT_TRUE( isGraphTooDeepError(output) ) << output;

        testing::internal::CaptureStdout();
        NatronTestPlugin::renderSequence(chain.output(), 1, 1);
        output = testing::internal::GetCapturedStdout();
        EXPECT_TRUE( isGraphTooDeepError(output) ) << output;
    }

    // without the limit, the chain renders
    int rendersBefore = NatronTestPlugin::counter("Generator.render");
    std::pair<bool, bool> result = renderFrameOnGlobalThreadPool( chain.lastFilter() );
    EXPECT_TRUE(result.first);
    EXPECT_LT( 0, NatronTestPlugin::counter("Generator.render") - rendersBefore );

    appPTR->clearAllCaches();
    rendersBefore = NatronTestPlugin::counter("Generator.render");
    testing::internal::CaptureStdout();
    NatronTestPlugin::renderSequence(chain.output(), 1, 1);
    std::string output = testing::internal::GetCapturedStdout();
    EXPECT_FALSE( isGraphTooDeepError(output) ) << output;
    EXPECT_LT( 0, NatronTestPlugin::counter("Generator.render") - rendersBefore );
}
