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
// pool now get a stack of getRenderThreadStackSize() bytes (at least NATRON_RENDER_THREAD_STACK_SIZE).
// On Windows that is the size committed when the thread starts, and the thread reserves the
// executable's stack reserve (NATRON_MAIN_THREAD_STACK_SIZE, 8 MB) if it is larger.

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <functional>
#include <future>
#include <iostream>
#include <limits>
#include <list>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

#include <QtCore/QTemporaryDir>
#include <QThread>
#include <QThreadPool>

#include "Engine/AppInstance.h"
#include "Engine/GraphDepthGuard.h"
#include "Engine/Hash64.h"
#include "Engine/KnobTypes.h"
#include "Engine/Node.h"
#include "Engine/Project.h"
#include "Engine/ThreadPool.h"

#include "OfxTestPlugin.h"

NATRON_NAMESPACE_USING

namespace {
// The chains of the tests that must render (600 MultiPAR filters, 1000 Dots) are a third as long in
// unoptimized (-O0) builds, whose stack frames are several times larger: there GraphDepthGuard, which stops
// a traversal before the stack overflows, reports a chain of about 600 MultiPAR nodes as too deep on macOS
// arm64 (#1865). The shorter chains use about as much stack as the long ones do in optimized builds. Any
// optimization level defines __OPTIMIZE__, so the builds at -O1 (the fast CI builds, the sanitizer builds)
// keep the long chains.
#ifdef __OPTIMIZE__
const int kStackFrameScale = 1;
#else
const int kStackFrameScale = 3;
#endif

// Number of filters in the chain: with 512 KB stacks, the render of this chain overflowed the stack
// of the render thread at a depth of about 440 nodes (in the getRegionOfDefinition recursion).
const int kChainLength = 600 / kStackFrameScale;

// Number of filters of a chain that is too deep for the stacks of Natron's threads (#1440): with
// 8 MB stacks, a render of about 3500 MultiPAR nodes overflowed it, and with GraphDepthGuard the
// limit is between 3000 and 3200 (macOS arm64). Such a chain must either render, or fail with the
// error "Node graph too deep", but never crash. The default length is above that limit on macOS
// arm64; on Linux, even 5000 filters render. NATRON_TEST_VERY_DEEP_CHAIN_LENGTH sets another
// length, e.g. 5000: creating and loading the nodes take most of the time, and grow faster than
// linearly (each new node looks for its name among the others), from 6 s for 3500 filters to 9 s
// for 5000 on macOS arm64.
int
getVeryDeepChainLength()
{
    const int kDefaultLength = 3500;
    const char* value = std::getenv("NATRON_TEST_VERY_DEEP_CHAIN_LENGTH");
    if (!value) {
        return kDefaultLength;
    }
    char* end = nullptr;
    const long length = std::strtol(value, &end, 10);
    if ( (end == value) || (*end != '\0') || (length < 1) || (length > 1000000) ) {
        std::cerr << "DeepGraphRender: NATRON_TEST_VERY_DEEP_CHAIN_LENGTH=" << value << " is not a length, using "
                  << kDefaultLength << std::endl;

        return kDefaultLength;
    }

    return (int)length;
}

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
//
// The generator and the filters are created and connected as one node tree, as when nodes are pasted
// or a PyPlug is created: connecting them one at a time runs the "input changed" action of each new
// filter with render args on the whole chain upstream of it, which is quadratic in the length of the
// chain (5000 filters took 161 s on Linux CI). Their input-related data and their hashes are then
// computed once for the whole chain. The output is connected as usual, with the chain upstream of it.
struct DeepChain
{
    std::vector<NodePtr> nodes; // from the generator to the output
    int length;

    DeepChain(int length = kChainLength,
              const char* filterID = kNatronTestPluginMultiPAR)
        : nodes()
        , length(length)
    {
        AppInstancePtr app = appPTR->getTopLevelInstance();
        ProjectPtr project = app->getProject();
        {
            CreatingNodeTreeFlag_RAII creatingNodeTree(app);
            NodePtr generator = createChainNode(kNatronTestPluginGenerator);
            if (!generator) {
                return;
            }
            nodes.push_back(generator);
            for (int i = 0; i < length; ++i) {
                NodePtr filter = createChainNode(filterID);
                if ( !filter || !project->connectNodes(0, nodes.back(), filter) ) {
                    return;
                }
                nodes.push_back(filter);
            }
        }
        project->forceComputeInputDependentDataOnAllTrees();
        // The connections made while the tree was created did not refresh the hashes downstream, and
        // forceComputeInputDependentDataOnAllTrees() doesn't either (pasting nodes doesn't refresh them).
        nodes.front()->incrementKnobsAge();
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

// Starts from an empty project (other tests may have left nodes in it) with a 16x16 format, and resets
// the project at the end, even if an ASSERT fails. The generator's RoD is the project format: with
// the default HD format, each render of a chain of 5000 filters took minutes.
struct SmallProject
{
    ProjectPtr project;

    SmallProject()
        : project( appPTR->getTopLevelInstance()->getProject() )
    {
        project->reset(false, true);
        // setOrAddProjectFormat() only changes the format if it may be auto-set, a user setting
        project->setAutoSetProjectFormatEnabled(true);
        project->setOrAddProjectFormat( Format(0, 0, 16, 16, "DeepGraphRender", 1.) );
    }

    ~SmallProject()
    {
        project->reset(false, true);
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
        NatronTestPlugin::reportMissing(kNatronTestPluginGenerator);

        return false;
    }
    if ( !NatronTestPlugin::isAvailable(kNatronTestPluginMultiPAR) ) {
        NatronTestPlugin::reportMissing(kNatronTestPluginMultiPAR);

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
    SmallProject smallProject;
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
    SmallProject smallProject;
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
    const std::size_t renderStackSize = std::max<std::size_t>(getRenderThreadStackSize(), NATRON_MAIN_THREAD_STACK_SIZE);
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
    SmallProject smallProject;
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
// The renders of checkVeryDeepChain()
enum class DeepChainRenders
{
    eAllThreads, // on the main thread, on a thread of the global thread pool and on a render thread
    eRenderThread, // on a render thread only, as NatronRenderer does
};

// Each operation on a very deep chain either works or fails with the "Node graph too deep" error.
void
checkVeryDeepChain(const NodePtr& output,
                   const NodePtr& lastFilter,
                   int length,
                   DeepChainRenders threads)
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

    // Each render starts from empty caches, so that it renders the whole chain on that thread, and
    // renders all the filters if it works.

    // render on the main thread
    if (threads == DeepChainRenders::eAllThreads) {
        appPTR->clearAllCaches();
        const int rendersBefore = NatronTestPlugin::counter("MultiPAR.render");
        testing::internal::CaptureStdout();
        ImagePtr image = NatronTestPlugin::renderFrame(lastFilter, 1.);
        std::string out = testing::internal::GetCapturedStdout();
        const int renders = NatronTestPlugin::counter("MultiPAR.render") - rendersBefore;
        EXPECT_TRUE( (image && (renders == length)) || (!image && isGraphTooDeepError(out)) ) << out;
        printStep(image ? "render on the main thread: OK" : "render on the main thread: too deep");
    }

    // render on a thread of the global thread pool
    if (threads == DeepChainRenders::eAllThreads) {
        appPTR->clearAllCaches();
        const int rendersBefore = NatronTestPlugin::counter("MultiPAR.render");
        testing::internal::CaptureStdout();
        std::pair<bool, bool> result = renderFrameOnGlobalThreadPool(lastFilter);
        std::string out = testing::internal::GetCapturedStdout();
        const int renders = NatronTestPlugin::counter("MultiPAR.render") - rendersBefore;
        EXPECT_TRUE(result.second) << "the render must run on a thread of the global thread pool";
        EXPECT_TRUE( (result.first && (renders == length)) || (!result.first && isGraphTooDeepError(out)) ) << out;
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

// A chain of getVeryDeepChainLength() filters, deeper than the stack allows: building it (the refresh
// of its input-related data, and the connection of its output), a change at its top, its
// reconnection (which refreshes the hash and the metadata downstream), its region of definition, its
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
    // declared after dir, so that the project is reset before the directory it was saved to is removed
    SmallProject smallProject;
    const ProjectPtr& project = smallProject.project;
    ASSERT_EQ( 0, (int)project->getNodes().size() );
    {
        Format format;
        project->getProjectDefaultFormat(&format);
        EXPECT_EQ( 16, format.width() );
        EXPECT_EQ( 16, format.height() );
    }
    const int length = getVeryDeepChainLength();
    std::string outputName, lastFilterName;

    printStep("start");
    {
        DeepChain chain(length);
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

        checkVeryDeepChain(chain.output(), chain.lastFilter(), length, DeepChainRenders::eAllThreads);

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
    EXPECT_EQ( length + 2, (int)project->getNodes().size() );

    // the loaded chain: its region of definition and one render (the other renders run the same
    // code as before the save)
    checkVeryDeepChain(output, lastFilter, length, DeepChainRenders::eRenderThread);
}

// The time to render a chain is linear in its length. renderRoI() walked the whole graph upstream of
// each node it rendered (in isFrameVaryingOrAnimated_Recursive(), and to copy the render args of every
// node of the project), so that the render of a chain was quadratic in its length: 5000 filters took
// 30 s on Linux CI.
TEST(DeepGraphRender, RenderTimeIsLinear)
{
    if ( !canRunTest() ) {
        return;
    }
    // Well below the limit of GraphDepthGuard, which is lower in debug builds (larger stack frames):
    // the other tests render kChainLength filters, and a chain of 2000 filters rendered on macOS arm64.
    const int shortLength = 50;
    const int longLength = 24 * shortLength;
    SmallProject smallProject;

    // the shortest of a few renders of a chain, each from empty caches. One chain at a time: the
    // render used to walk every node of the project for each rendered node.
    const auto renderTime = [](int length) {
        DeepChain chain(length);
        EXPECT_TRUE( chain.isValid() );
        double best = std::numeric_limits<double>::infinity();
        for (int i = 0; chain.isValid() && i < 5; ++i) {
            appPTR->clearAllCaches();
            const std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
            EXPECT_TRUE( bool( NatronTestPlugin::renderFrame(chain.lastFilter(), 1.) ) );
            best = std::min( best, std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count() );
        }

        return best;
    };
    const double shortTime = renderTime(shortLength);
    const double longTime = renderTime(longLength);
    // 24 times longer if linear, 576 times if quadratic. Measured on macOS arm64: 30 to 38 times, and
    // 140 times with only the walk of isFrameVaryingOrAnimated_Recursive() quadratic.
    EXPECT_LT(longTime, 80 * shortTime) << "rendering " << shortLength << " filters took " << shortTime
                                        << " s, and " << longLength << " filters " << longTime << " s";
    std::cerr << "DeepGraphRender: rendering " << shortLength << " filters took " << shortTime << " s, and "
              << longLength << " filters " << longTime << " s" << std::endl;
}

// The region of definition and the frame range of a Dot (and of the other effects that use the default
// implementations of EffectInstance) are those of its input, from the cache of the input: the RoD of
// the input was cached under the hash of the Dot, which the lookups of the input never find, and its
// frame range was not cached, so that the RoD and the frame range of a chain of Dots were computed
// again over the whole chain upstream of each Dot, quadratic in the length of the chain.
TEST(DeepGraphRender, DotsUseTheCacheOfTheirInput)
{
    if ( !canRunTest() ) {
        return;
    }
    SmallProject smallProject;
    const int length = 3;
    DeepChain chain(length, PLUGINID_NATRON_DOT);
    ASSERT_TRUE( chain.isValid() );
    // new hashes: nothing is cached yet
    chain.nodes.front()->incrementKnobsAge();

    const EffectInstancePtr lastDot = chain.lastFilter()->getEffectInstance();
    RectD rod;
    ASSERT_EQ( eStatusOK, lastDot->getRegionOfDefinition_public(lastDot->getHash(), 1., RenderScale::identity, ViewIdx(0), &rod, nullptr) );
    // the generator and the Dots upstream of the last one
    for (int i = 0; i < length; ++i) {
        const EffectInstancePtr effect = chain.nodes[i]->getEffectInstance();
        RectD cached;
        EXPECT_EQ( eStatusOK, effect->getRegionOfDefinitionFromCache(effect->getHash(), 1., RenderScale::identity, ViewIdx(0), &cached, nullptr) )
            << "the RoD of " << chain.nodes[i]->getScriptName() << " is not in its cache";
        EXPECT_EQ(rod, cached);
    }

    // the frame range of each Dot, from the last one: the time domain of the generator is computed once
    const int before = NatronTestPlugin::counter("Generator.getTimeDomain");
    for (int i = length; i > 0; --i) {
        const EffectInstancePtr effect = chain.nodes[i]->getEffectInstance();
        double first = 0., last = 0.;
        effect->getFrameRange_public(effect->getHash(), &first, &last);
    }
    EXPECT_EQ( 1, NatronTestPlugin::counter("Generator.getTimeDomain") - before );
}

// A chain of Dot nodes, which are identities: renderRoI goes straight to the input. Rendering very
// deep Dot chains is still quadratic (tryConcatenateTransforms() walks up through every Dot, an
// identity transform, for each node), so this uses a shorter chain (the test takes 0.6 s with 1000
// Dots on macOS arm64, 1.1 s with 1500), still deeper than the 440 nodes that overflowed 512 KB
// stacks (#1434), and a lower maximum depth.
TEST(DeepGraphRender, DotChain)
{
    if ( !canRunTest() ) {
        return;
    }
    SmallProject smallProject;
    const int length = 1000 / kStackFrameScale;
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

namespace {
// The hash propagation of Node::computeHash() was a recursion until #1611. This is that recursion, written with the
// public API, so that the test checks that the iterative version gives the same hashes.

typedef std::map<const Node*, U64> HashMap;

// Node::computeHashInternal() for the nodes of these tests (no Viewer, no RotoPaint), where the hash
// of each input is read from hashes
U64
referenceNodeHash(const NodePtr& node,
                  const HashMap& hashes)
{
    Hash64 hash;

    hash.append( node->getKnobsAge() );
    for (int i = 0; i < node->getNInputs(); ++i) {
        NodePtr input = node->getInput(i);
        if (input) {
            hash.append( hashes.at( input.get() ) + (U32)i );
        }
    }
    Hash64_appendQString( &hash, QString::fromUtf8( node->getScriptName().c_str() ) );
    hash.append( node->getApp()->getProject()->getProjectCreationTime() );
    hash.computeHash();

    return hash.value();
}

// The former Node::computeHashRecursive(): hashes holds the hash of every node before the change,
// and gets the new ones.
void
referenceComputeHashRecursive(const NodePtr& node,
                              std::list<const Node*>& marked,
                              HashMap& hashes)
{
    if ( std::find( marked.begin(), marked.end(), node.get() ) != marked.end() ) {
        return;
    }
    const U64 oldHash = hashes.at( node.get() );
    const U64 newHash = referenceNodeHash(node, hashes);
    hashes[node.get()] = newHash;
    marked.push_back( node.get() );
    if (oldHash == newHash) {
        return;
    }
    NodesList outputs;
    node->getOutputsWithGroupRedirection(outputs);
    for (NodesList::iterator it = outputs.begin(); it != outputs.end(); ++it) {
        referenceComputeHashRecursive(*it, marked, hashes);
    }
}

ParallelRenderArgsSetter*
makeRenderArgs(const NodePtr& root,
               const AbortableRenderInfoPtr& abortInfo)
{
    return new ParallelRenderArgsSetter( 1., ViewIdx(0),
                                         true, // isRenderUserInteraction
                                         false, // isSequential
                                         abortInfo,
                                         root, // tree root
                                         0, // texture index
                                         root->getApp()->getTimeLine().get(),
                                         NodePtr(), // rotoPaint node
                                         false, // isAnalysis
                                         false, // isDraft
                                         RenderStatsPtr() );
}

// Sets up the render args of the tree of root on the calling thread, and checks the visits count of
// the first and last node of the chain: they are only reached through the inputs, once.
bool
setRenderArgsOfChain(const NodePtr& root,
                     const NodePtr& first)
{
    AbortableRenderInfoPtr abortInfo = AbortableRenderInfo::create(false, 0);
    std::unique_ptr<ParallelRenderArgsSetter> setter( makeRenderArgs(root, abortInfo) );
    ParallelRenderArgsPtr rootArgs = root->getEffectInstance()->getParallelRenderArgsTLS();
    ParallelRenderArgsPtr firstArgs = first->getEffectInstance()->getParallelRenderArgsTLS();

    return rootArgs && firstArgs && (rootArgs->visitsCount == 1) && (firstArgs->visitsCount == 1);
}

// Sets up the render args of the tree of root on the calling thread, as a render does, and asks first whether to cache
// its output, as renderRoI() does: Node::shouldCacheOutput() walks downstream of first through every identity node (the
// Dot nodes of the chain) to the first node that is not one (addIdentityNodesDownstream(), a recursion before #1611).
// Returns true once it is done (whether to cache doesn't matter here).
bool
askCachingOfChain(const NodePtr& root,
                  const NodePtr& first)
{
    AbortableRenderInfoPtr abortInfo = AbortableRenderInfo::create(false, 0);
    std::unique_ptr<ParallelRenderArgsSetter> setter( makeRenderArgs(root, abortInfo) );

    first->shouldCacheOutput(false /*isFrameVaryingOrAnimated*/, 1., ViewIdx(0), 1 /*visitsCount*/);

    return true;
}
} // anon namespace

// The iterative traversals (#1611) give the same render args and the same hashes as the former
// recursions, on a graph where the order of the visits matters: the hash of a node reached through
// two paths is computed when the first path reaches it, with the hash that the other path has not
// updated yet.
//
//  Generator --> A (MultiPAR) --> M (MultiPAR, inputs A and B) --> P (Params) --> Dot --> DiskCache
//            \-> B (MultiPAR) -/
//
// An expression of P reads X (Params, not connected).
TEST(DeepGraphRender, IterativeTraversalsKeepResults)
{
    if ( !canRunTest() ) {
        return;
    }
    if ( !NatronTestPlugin::isAvailable(kNatronTestPluginParams) ) {
        NatronTestPlugin::reportMissing(kNatronTestPluginParams);

        return;
    }
    SmallProject smallProject;
    std::vector<NodePtr> nodes;
    struct Destroy
    {
        std::vector<NodePtr>& nodes;
        ~Destroy()
        {
            for (std::vector<NodePtr>::iterator it = nodes.begin(); it != nodes.end(); ++it) {
                const KnobsVec& knobs = (*it)->getKnobs();
                for (KnobsVec::const_iterator k = knobs.begin(); k != knobs.end(); ++k) {
                    for (int i = 0; i < (*k)->getDimension(); ++i) {
                        if ( !(*k)->getExpression(i).empty() ) {
                            (*k)->clearExpression(i, true);
                        }
                    }
                }
            }
            for (std::vector<NodePtr>::reverse_iterator it = nodes.rbegin(); it != nodes.rend(); ++it) {
                (*it)->destroyNode(true, false);
            }
        }
    } destroy = { nodes };
    const char* ids[] = { kNatronTestPluginGenerator, kNatronTestPluginMultiPAR, kNatronTestPluginMultiPAR, kNatronTestPluginMultiPAR,
                          kNatronTestPluginParams, PLUGINID_NATRON_DOT, PLUGINID_NATRON_DISKCACHE, kNatronTestPluginParams };
    for (std::size_t i = 0; i < sizeof(ids) / sizeof(ids[0]); ++i) {
        NodePtr node = createChainNode(ids[i]);
        ASSERT_TRUE( bool(node) ) << ids[i];
        nodes.push_back(node);
    }
    const NodePtr generator = nodes[0], a = nodes[1], b = nodes[2], m = nodes[3], p = nodes[4], dot = nodes[5], output = nodes[6], x = nodes[7];
    ASSERT_LE( 2, m->getNInputs() );
    ASSERT_LE( 1, p->getNInputs() );
    ProjectPtr project = generator->getApp()->getProject();
    ASSERT_TRUE( project->connectNodes(0, generator, a) );
    ASSERT_TRUE( project->connectNodes(0, generator, b) );
    ASSERT_TRUE( project->connectNodes(0, a, m) );
    ASSERT_TRUE( project->connectNodes(1, b, m) );
    ASSERT_TRUE( project->connectNodes(0, m, p) );
    ASSERT_TRUE( project->connectNodes(0, p, dot) );
    ASSERT_TRUE( project->connectNodes(0, dot, output) );
    KnobDoublePtr pTo = std::dynamic_pointer_cast<KnobDouble>( p->getKnobByName("doubleTo") );
    ASSERT_TRUE( bool(pTo) );
    pTo->setExpression(0, x->getScriptName_mt_safe() + ".doubleFrom.get()",
                       false /*hasRetVariable*/, false /*failIfInvalid*/);
    ASSERT_FALSE( pTo->getExpression(0).empty() );

    // the render args, as with the former recursion: the visits count of a node upstream is its number of connections
    // downstream (the generator has two), and X, read by an expression, has render args but no visit
    {
        AbortableRenderInfoPtr abortInfo = AbortableRenderInfo::create(false, 0);
        std::unique_ptr<ParallelRenderArgsSetter> setter( makeRenderArgs(output, abortInfo) );
        const int expectedVisits[] = { 2, 1, 1, 1, 1, 1, 1, 0 }; // in the order of nodes
        for (std::size_t i = 0; i < nodes.size(); ++i) {
            ParallelRenderArgsPtr args = nodes[i]->getEffectInstance()->getParallelRenderArgsTLS();
            ASSERT_TRUE( bool(args) ) << nodes[i]->getScriptName();
            EXPECT_EQ(expectedVisits[i], args->visitsCount) << nodes[i]->getScriptName();
        }
    }

    // the hashes: change the generator, and compare with the former recursion
    HashMap hashes;
    for (std::vector<NodePtr>::const_iterator it = nodes.begin(); it != nodes.end(); ++it) {
        hashes[it->get()] = (*it)->getHashValue();
    }
    // check referenceNodeHash(): the graph is up to date
    for (std::vector<NodePtr>::const_iterator it = nodes.begin(); it != nodes.end(); ++it) {
        ASSERT_EQ( hashes[it->get()], referenceNodeHash(*it, hashes) ) << (*it)->getScriptName();
    }
    generator->incrementKnobsAge();
    {
        std::list<const Node*> marked;
        referenceComputeHashRecursive(generator, marked, hashes);
    }
    HashMap actual;
    for (std::vector<NodePtr>::const_iterator it = nodes.begin(); it != nodes.end(); ++it) {
        actual[it->get()] = (*it)->getHashValue();
    }
    for (std::vector<NodePtr>::const_iterator it = nodes.begin(); it != nodes.end(); ++it) {
        EXPECT_EQ( hashes[it->get()], actual[it->get()] ) << (*it)->getScriptName();
    }
    EXPECT_EQ( x->getHashValue(), hashes[x.get()] );
    // The test depends on the order: M was computed through A, before B changed.
    EXPECT_NE( actual[m.get()], referenceNodeHash(m, actual) ) << "the order of the visits does not matter in this graph";
}

namespace {
// The length of the Dot chain of VeryDeepDotChainRenderArgsAndHash: 1.6 times the depth at which the former recursions
// overflowed the stack with little stack left (1871 nodes for the hash propagation and 1570 for the upstream traversal
// of ParallelRenderArgsSetter, macOS arm64; see #1627).
const int kVeryDeepDotChainLength = 3000;

// The stack size of the thread that sets up the render args in VeryDeepDotChainRenderArgsAndHash
const unsigned int kSmallStackSize = 256 * 1024;

// A thread with a stack of kSmallStackSize bytes, which runs a function
class SmallStackThread
    : public QThread
{
public:
    explicit SmallStackThread(const std::function<void()>& f)
        : QThread()
        , _f(f)
    {
        setStackSize(kSmallStackSize);
    }

private:
    void run() OVERRIDE
    {
        _f();
    }

    std::function<void()> _f;
};

// see callWithLittleStackLeft()
char* volatile gStackChunk = nullptr;

// Calls f with about 256 KB of stack left on the main thread, whose stack size can't be changed, to overflow its stack
// with a short chain: first uses the stack down to GraphDepthGuard's safety margin, then 768 KB of that margin, which is
// 1 MB on stacks of 8 MB or more (the main thread's on every platform, see StackBounds).
NEVER_INLINE void
callWithLittleStackLeft(const std::function<void()>& f,
                        int marginChunksToUse = 48)
{
    const std::size_t kChunkSize = 16 * 1024;
    const bool aboveMargin = GraphDepthGuard::getStackLeft() >= 2 * kChunkSize;

    if ( !aboveMargin && (marginChunksToUse <= 0) ) {
        f();

        return;
    }
    char chunk[kChunkSize];
    // The address of the chunk escapes, so that the compiler keeps it on the stack, and the store after the call keeps
    // it from being a tail call that would reuse this frame.
    gStackChunk = chunk;
    callWithLittleStackLeft(f, aboveMargin ? marginChunksToUse : marginChunksToUse - 1);
    gStackChunk = nullptr;
}
} // anon namespace

// The render args of a very deep chain are set up on every kind of thread, a change at its top changes the hash of
// every node downstream, and shouldCacheOutput() walks down the chain, without overflowing the stack (#1611). To keep
// the chain short, the traversals run on a thread with a small stack, or with little stack left on the main thread.
TEST(DeepGraphRender, VeryDeepDotChainRenderArgsAndHash)
{
    if ( !canRunTest() ) {
        return;
    }
    const int length = kVeryDeepDotChainLength;
    SmallProject smallProject;
    printStep("start");
    DeepChain chain(length, PLUGINID_NATRON_DOT);
    ASSERT_TRUE( chain.isValid() );
    printStep("Dot chain built");
    const NodePtr root = chain.output();
    const NodePtr first = chain.nodes.front();

    // the hash propagation, downstream, on the main thread (where the hash is always computed)
    {
        const U64 hashBefore = root->getHashValue();
        callWithLittleStackLeft( [&first]() {
            first->incrementKnobsAge();
        } );
        EXPECT_NE( hashBefore, root->getHashValue() );
        printStep("hash changed");
    }

    // the upstream traversal of ParallelRenderArgsSetter, on the main thread
    {
        bool set = false;
        callWithLittleStackLeft( [&root, &first, &set]() {
            set = setRenderArgsOfChain(root, first);
        } );
        EXPECT_TRUE(set);
        printStep("render args on the main thread");
    }

    // on a thread with a small stack
    {
        bool set = false;
        SmallStackThread thread( [&root, &first, &set]() {
            set = setRenderArgsOfChain(root, first);
        } );
        thread.start();
        thread.wait();
        EXPECT_TRUE(set);
        printStep("render args on a thread with a small stack");
    }

    // the downstream walk of Node::shouldCacheOutput(), from the top of the chain, on the main thread and on a thread
    // with a small stack
    {
        bool asked = false;
        callWithLittleStackLeft( [&root, &first, &asked]() {
            asked = askCachingOfChain(root, first);
        } );
        EXPECT_TRUE(asked);
        printStep("shouldCacheOutput on the main thread");
    }
    {
        bool asked = false;
        SmallStackThread thread( [&root, &first, &asked]() {
            asked = askCachingOfChain(root, first);
        } );
        thread.start();
        thread.wait();
        EXPECT_TRUE(asked);
        printStep("shouldCacheOutput on a thread with a small stack");
    }

    // on a thread of the global thread pool
    {
        std::promise<bool> promise;
        std::future<bool> set = promise.get_future();
        QThreadPool::globalInstance()->start( [root, first, &promise]() {
            promise.set_value( setRenderArgsOfChain(root, first) );
        } );
        EXPECT_TRUE( set.get() );
        printStep("render args on the global thread pool");
    }

    // on a render thread: a render on disk sets up the render args before anything else (#1564), so
    // it reaches the depth limit afterwards, in the region of definition
    {
        MaxGraphDepthSetter limit(100);
        testing::internal::CaptureStdout();
        NatronTestPlugin::renderSequence(root, 1, 1);
        const std::string output = testing::internal::GetCapturedStdout();
        EXPECT_TRUE( isGraphTooDeepError(output) ) << output;
        printStep("render on a render thread");
    }
}

namespace {
// The nodes created by a test, destroyed in the reverse order at the end, even if an ASSERT fails
struct TestNodes
{
    std::vector<NodePtr> nodes;

    ~TestNodes()
    {
        for (std::vector<NodePtr>::reverse_iterator it = nodes.rbegin(); it != nodes.rend(); ++it) {
            (*it)->destroyNode(true, false);
        }
    }

    NodePtr add(const char* pluginID)
    {
        NodePtr node = createChainNode(pluginID);

        if (node) {
            nodes.push_back(node);
        }

        return node;
    }
};

// The number of diamonds of NodesOnPathsUpstreamOfDiamonds: a walk that visits each node once per path to it makes
// about 2^kDiamondsCount visits, and never finishes.
const int kDiamondsCount = 40;

// The diamonds of the reference walks, which make about 2^kReferenceDiamondsCount visits
const int kReferenceDiamondsCount = 10;

// The former isRotoPaintNodeInputRecursive() (ViewerInstance.cpp) and isBranchConnectedToRotoNodeRecursive()
// (RotoPaintInteract.cpp): whether target is node or upstream of it. Counts its calls.
bool
referenceLeadsTo(const NodePtr& node,
                 const Node* target,
                 int* calls)
{
    ++*calls;
    if (node.get() == target) {
        return true;
    }
    for (int i = 0; i < node->getNInputs(); ++i) {
        NodePtr input = node->getInput(i);
        if ( input && referenceLeadsTo(input, target, calls) ) {
            return true;
        }
    }

    return false;
}

// The former updateLastStrokeDataRecursively() (ViewerInstance.cpp), which appends the nodes it updates to updated
void
referenceUpdateRecursively(const NodePtr& node,
                           const Node* target,
                           NodesList* updated,
                           int* calls)
{
    if ( !referenceLeadsTo(node, target, calls) ) {
        return;
    }
    updated->push_back(node);
    if (node.get() == target) {
        return;
    }
    for (int i = 0; i < node->getNInputs(); ++i) {
        NodePtr input = node->getInput(i);
        if (input) {
            referenceUpdateRecursively(input, target, updated, calls);
        }
    }
}

// The nodes, each once, in the order in which they first appear
NodesList
firstOccurrences(const NodesList& nodes)
{
    NodesList first;

    for (NodesList::const_iterator it = nodes.begin(); it != nodes.end(); ++it) {
        if ( std::find(first.begin(), first.end(), *it) == first.end() ) {
            first.push_back(*it);
        }
    }

    return first;
}

std::string
scriptNames(const NodesList& nodes)
{
    std::string names;

    for (NodesList::const_iterator it = nodes.begin(); it != nodes.end(); ++it) {
        names += (*it)->getScriptName() + ' ';
    }

    return names;
}
} // anon namespace

// The walks of a RotoPaint stroke (#1628) visit each node once: Node::getNodesOnPathsUpstreamTo(), which updates the
// paint stroke data of the nodes between the Viewer and the RotoPaint node during a stroke, and tells whether the
// Viewer's inputs lead to the RotoPaint node, on a chain of diamonds where the former recursions never finished. It
// gives the nodes in the order of the first visits of the former recursion.
//
//  Generator --> Dot A1 --> MultiPAR M1 (inputs A1, B1) --> Dot A2 --> M2 ... --> Mk --> MultiPAR Top (inputs Mk, C)
//            \-> Dot B1 -/                              \-> Dot B2 -/                 Generator X --> Dot C -/
TEST(DeepGraphRender, NodesOnPathsUpstreamOfDiamonds)
{
    if ( !canRunTest() ) {
        return;
    }
    SmallProject smallProject;
    TestNodes nodes;
    AppInstancePtr app = appPTR->getTopLevelInstance();
    ProjectPtr project = app->getProject();
    std::vector<NodePtr> a(1), b(1), m; // a[i], b[i] and m[i] for i >= 1; m[0] is the generator
    NodePtr x, c, top;
    {
        CreatingNodeTreeFlag_RAII creatingNodeTree(app);
        m.push_back( nodes.add(kNatronTestPluginGenerator) );
        ASSERT_TRUE( bool( m.back() ) );
        for (int i = 1; i <= kDiamondsCount; ++i) {
            a.push_back( nodes.add(PLUGINID_NATRON_DOT) );
            b.push_back( nodes.add(PLUGINID_NATRON_DOT) );
            m.push_back( nodes.add(kNatronTestPluginMultiPAR) );
            ASSERT_TRUE( a[i] && b[i] && m[i] );
            ASSERT_LE( 2, m[i]->getNInputs() );
            ASSERT_TRUE( project->connectNodes(0, m[i - 1], a[i]) );
            ASSERT_TRUE( project->connectNodes(0, m[i - 1], b[i]) );
            ASSERT_TRUE( project->connectNodes(0, a[i], m[i]) );
            ASSERT_TRUE( project->connectNodes(1, b[i], m[i]) );
        }
        x = nodes.add(kNatronTestPluginGenerator);
        c = nodes.add(PLUGINID_NATRON_DOT);
        top = nodes.add(kNatronTestPluginMultiPAR);
        ASSERT_TRUE( x && c && top );
        ASSERT_TRUE( project->connectNodes(0, x, c) );
        ASSERT_TRUE( project->connectNodes(0, m.back(), top) );
        ASSERT_TRUE( project->connectNodes(1, c, top) );
    }
    project->forceComputeInputDependentDataOnAllTrees();
    const NodePtr& generator = m[0];

    const std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
    const NodesList toGenerator = top->getNodesOnPathsUpstreamTo( generator.get() );
    const int middle = kDiamondsCount / 2;
    const NodesList toMiddle = top->getNodesOnPathsUpstreamTo( m[middle].get() );
    const NodesList toX = top->getNodesOnPathsUpstreamTo( x.get() );
    const NodesList fromMToX = m.back()->getNodesOnPathsUpstreamTo( x.get() );
    const NodesList toTop = top->getNodesOnPathsUpstreamTo( top.get() );
    const NodesList fromGeneratorToTop = generator->getNodesOnPathsUpstreamTo( top.get() );
    const long long elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count();
    EXPECT_LT(elapsedMs, 1000);

    // the order of the former recursion: down the A branches, then the B branches on the way back
    NodesList expected;
    expected.push_back(top);
    for (int i = kDiamondsCount; i >= 1; --i) {
        expected.push_back(m[i]);
        expected.push_back(a[i]);
    }
    expected.push_back(generator);
    for (int i = 1; i <= kDiamondsCount; ++i) {
        expected.push_back(b[i]);
    }
    EXPECT_EQ( scriptNames(expected), scriptNames(toGenerator) );

    expected.clear();
    expected.push_back(top);
    for (int i = kDiamondsCount; i > middle; --i) {
        expected.push_back(m[i]);
        expected.push_back(a[i]);
    }
    expected.push_back(m[middle]);
    for (int i = middle + 1; i <= kDiamondsCount; ++i) {
        expected.push_back(b[i]);
    }
    EXPECT_EQ( scriptNames(expected), scriptNames(toMiddle) );

    expected.clear();
    expected.push_back(top);
    expected.push_back(c);
    expected.push_back(x);
    EXPECT_EQ( scriptNames(expected), scriptNames(toX) );
    EXPECT_TRUE( fromMToX.empty() ) << scriptNames(fromMToX);
    EXPECT_EQ( scriptNames( NodesList(1, top) ), scriptNames(toTop) );
    EXPECT_TRUE( fromGeneratorToTop.empty() ) << scriptNames(fromGeneratorToTop);

    // the same nodes, in the same order, as the former recursion, on a part of the chain where it finishes
    const NodePtr& root = m[kReferenceDiamondsCount];
    const NodePtr targets[] = { generator, m[kReferenceDiamondsCount / 2], m[kReferenceDiamondsCount], x };
    for (std::size_t i = 0; i < sizeof(targets) / sizeof(targets[0]); ++i) {
        NodesList updated;
        int calls = 0;
        referenceUpdateRecursively(root, targets[i].get(), &updated, &calls);
        EXPECT_EQ( scriptNames( firstOccurrences(updated) ), scriptNames( root->getNodesOnPathsUpstreamTo( targets[i].get() ) ) )
            << "to " << targets[i]->getScriptName();
        if (targets[i] == generator) {
            // the former recursion is exponential on this graph
            EXPECT_LE(1 << kReferenceDiamondsCount, calls);
        }
    }
}

// The refresh of the input-related data (metadata, layer menus...) on project load and after a connection doesn't
// overflow the stack in a very deep chain (#1628). On project load, the nodes are split into trees, and each tree is
// refreshed from its output, which first refreshes its dirty inputs; after a connection, the data of every node
// downstream is marked dirty, then refreshed, from the top. These walks were recursions, one stack frame per node of the
// chain. As in VeryDeepDotChainRenderArgsAndHash, they run with little stack left on the main thread, to keep the chain
// short.
TEST(DeepGraphRender, VeryDeepDotChainInputRelatedData)
{
    if ( !canRunTest() ) {
        return;
    }
    SmallProject smallProject;
    printStep("start");
    DeepChain chain(kVeryDeepDotChainLength, PLUGINID_NATRON_DOT);
    ASSERT_TRUE( chain.isValid() );
    printStep("Dot chain built");
    const NodePtr generator = chain.nodes[0];
    const NodePtr firstDot = chain.nodes[1];
    ProjectPtr project = generator->getApp()->getProject();
    const auto countDirty = [&chain]() {
        int dirty = 0;
        for (std::vector<NodePtr>::const_iterator it = chain.nodes.begin(); it != chain.nodes.end(); ++it) {
            if ( (*it)->isInputRelatedDataDirty() ) {
                ++dirty;
            }
        }

        return dirty;
    };

    // as on project load: from the output, upstream
    callWithLittleStackLeft( [&project]() {
        project->forceComputeInputDependentDataOnAllTrees();
    } );
    EXPECT_EQ( 0, countDirty() );
    printStep("input-related data refreshed upstream");

    // after a connection at the top (Node::endInputEdition()): downstream. The former recursions use less stack per node
    // there: 3000 nodes didn't overflow 256 KB, so this leaves 128 KB (56 of the 64 chunks of the margin). The connection
    // itself is not made here: it runs other recursive walks downstream (Node::hasOutputNodesConnected(), for example).
    const int kDownstreamMarginChunks = 56;
    callWithLittleStackLeft( [&firstDot]() {
        firstDot->forceRefreshAllInputRelatedData();
    }, kDownstreamMarginChunks );
    EXPECT_EQ( 0, countDirty() );
    printStep("input-related data refreshed downstream");
}
