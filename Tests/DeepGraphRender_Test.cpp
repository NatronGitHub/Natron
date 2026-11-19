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
// pool now have a stack of NATRON_RENDER_THREAD_STACK_SIZE bytes.

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <vector>

#include <gtest/gtest.h>

#include <QThreadPool>
#include <QtConcurrentRun> // QtConcurrent on Qt5

#include "Engine/ThreadPool.h"

#include "OfxTestPlugin.h"

NATRON_NAMESPACE_USING

namespace {
// Number of filters in the chain: with 512 KB stacks, the render of this chain overflowed the stack
// of the render thread at a depth of about 440 nodes (in the getRegionOfDefinition recursion).
const int kChainLength = 600;

// net.natron.test.Generator -> kChainLength x net.natron.test.MultiPAR (a filter) -> DiskCache
struct DeepChain
{
    std::vector<NodePtr> nodes; // from the generator to the output

    DeepChain()
    {
        NodePtr generator = NatronTestPlugin::createNode(kNatronTestPluginGenerator);
        if (!generator) {
            return;
        }
        nodes.push_back(generator);
        ProjectPtr project = generator->getApp()->getProject();
        for (int i = 0; i < kChainLength; ++i) {
            NodePtr filter = NatronTestPlugin::createNode(kNatronTestPluginMultiPAR);
            if ( !filter || !project->connectNodes(0, nodes.back(), filter) ) {
                return;
            }
            nodes.push_back(filter);
        }
        NodePtr output = NatronTestPlugin::createNode(PLUGINID_NATRON_DISKCACHE);
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
        return (int)nodes.size() == kChainLength + 2;
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

// The stack size applies to the threads that the global pool creates after AppManager::load().
TEST(DeepGraphRender, GlobalThreadPoolStackSize)
{
    EXPECT_EQ( (uint)NATRON_RENDER_THREAD_STACK_SIZE, QThreadPool::globalInstance()->stackSize() );
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
    const NodePtr node = chain.lastFilter();
    QFuture<bool> rendered = QtConcurrent::run( [node]() {
        return bool( NatronTestPlugin::renderFrame(node, 1.) );
    } );
    EXPECT_TRUE( rendered.result() );
    EXPECT_EQ( kChainLength, NatronTestPlugin::counter("MultiPAR.render") - rendersBefore );
}
