/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2023 The Natron developers
 * (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
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

// Regression tests for deadlocks between the main thread and render threads in groups (related to
// https://github.com/NatronGitHub/Natron/issues/814, "Application freeze when copying a node with enabled
// postage stamp").
//
// A render thread pulling an image through a GroupInput goes from the node connected to the GroupInput
// (Node::getInput(), which held that node's inputsMutex while applying the group redirections) to
// NodeGroup::getRealInputForInput() (which held the group's nodesLock while calling Node::getInput() on the
// group node, which takes the group node's inputsMutex). The main thread took the same locks in the other order:
// - Node::initializeInputs() held the group node's inputsMutex while calling NodeGroup::isInputMask(), which
//   takes nodesLock;
// - NodeGroup::notifyNodeDeactivated() (deleting or deactivating a GroupInput) held nodesLock while
//   disconnecting the group input, which recomputes the hash of the nodes connected to the GroupInputs and
//   takes their inputsMutex.
// Each thread then waited for a lock held by the other one, freezing the GUI.

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
#include <iostream>
#include <list>
#include <string>
#include <thread>

#include <gtest/gtest.h>

#include "Engine/AppInstance.h"
#include "Engine/AppManager.h"
#include "Engine/CreateNodeArgs.h"
#include "Engine/EffectInstance.h"
#include "Engine/Node.h"
#include "Engine/NodeGroup.h"
#include "Engine/Project.h"

NATRON_NAMESPACE_USING

namespace {
NodePtr
createNodeIn(const AppInstancePtr& app,
             const std::string& pluginID,
             const NodeCollectionPtr& group,
             bool createInitialNodes)
{
    CreateNodeArgs args(pluginID, group);
    args.setProperty<bool>(kCreateNodeArgsPropNodeGroupDisableCreateInitialNodes, !createInitialNodes);
    args.setProperty<bool>(kCreateNodeArgsPropAddUndoRedoCommand, false);
    args.setProperty<bool>(kCreateNodeArgsPropAutoConnect, false);

    return app->createNode(args);
}

// If the threads deadlock, nothing else can report it: abort after a while.
// Stops (and joins its thread) when destroyed, also when the test exits early.
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
                std::cerr << testName << ": deadlock? (total time of 60 s exceeded)" << std::endl;
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

// Calls f() in a loop on another thread, as a render thread does, until destroyed (also when the test exits early).
class RenderThreadLoop
{
    std::atomic<bool> _stop;
    std::thread _thread;

public:
    explicit RenderThreadLoop(const std::function<void()>& f)
        : _stop(false)
    {
        _thread = std::thread([this, f]() {
            while (!_stop) {
                f();
            }
        });
    }

    ~RenderThreadLoop()
    {
        _stop = true;
        if ( _thread.joinable() ) {
            _thread.join();
        }
    }
};
} // anonymous namespace

TEST(NodeGroup, InitializeInputsDoesNotDeadlockWithRenderThread)
{
    AppInstancePtr app = appPTR->getTopLevelInstance();
    ASSERT_TRUE( bool(app) );

    NodePtr groupNode = createNodeIn(app, PLUGINID_NATRON_GROUP, app->getProject(), false);
    ASSERT_TRUE( bool(groupNode) );
    NodeGroupPtr group = std::dynamic_pointer_cast<NodeGroup>( groupNode->getEffectInstance() );
    ASSERT_TRUE( bool(group) );
    NodePtr inputNode = createNodeIn(app, PLUGINID_NATRON_INPUT, group, false);
    ASSERT_TRUE( bool(inputNode) );
    ASSERT_EQ(1, groupNode->getNInputs());

    {
        Watchdog watchdog("NodeGroup.InitializeInputsDoesNotDeadlockWithRenderThread");
        // What a render thread does when it pulls an image through the GroupInput.
        RenderThreadLoop renderThread([&group, &inputNode]() {
            (void)group->getRealInputForInput(false, inputNode);
        });

        // What the main thread does when a GroupInput's mask/optional state or name changes.
        for (int i = 0; i < 20000; ++i) {
            groupNode->initializeInputs();
        }
    }

    EXPECT_EQ(1, groupNode->getNInputs());

    groupNode->destroyNode(true, false);
}

TEST(NodeGroup, DeactivateGroupInputDoesNotDeadlockWithRenderThread)
{
    AppInstancePtr app = appPTR->getTopLevelInstance();
    ASSERT_TRUE( bool(app) );

    // source -> [ group: inputA, inputB -> dot ]
    NodePtr source = createNodeIn(app, PLUGINID_NATRON_DOT, app->getProject(), false);
    ASSERT_TRUE( bool(source) );
    NodePtr groupNode = createNodeIn(app, PLUGINID_NATRON_GROUP, app->getProject(), false);
    ASSERT_TRUE( bool(groupNode) );
    NodeGroupPtr group = std::dynamic_pointer_cast<NodeGroup>( groupNode->getEffectInstance() );
    ASSERT_TRUE( bool(group) );
    NodePtr inputA = createNodeIn(app, PLUGINID_NATRON_INPUT, group, false);
    ASSERT_TRUE( bool(inputA) );
    NodePtr inputB = createNodeIn(app, PLUGINID_NATRON_INPUT, group, false);
    ASSERT_TRUE( bool(inputB) );
    NodePtr dot = createNodeIn(app, PLUGINID_NATRON_DOT, group, false);
    ASSERT_TRUE( bool(dot) );
    ASSERT_TRUE( dot->connectInput(inputB, 0) );
    ASSERT_EQ(2, groupNode->getNInputs());

    // Connects the source to every input of the group, so that deactivating a GroupInput disconnects
    // the corresponding input of the group.
    const auto connectGroupInputs = [&groupNode, &source]() {
        for (int i = 0; i < groupNode->getNInputs(); ++i) {
            if ( !groupNode->getRealInput(i) ) {
                groupNode->connectInput(source, i);
            }
        }
    };
    connectGroupInputs();
    ASSERT_EQ( source, dot->getInput(0) ); // through inputB and the group

    std::atomic<bool> unexpectedInput(false);
    {
        Watchdog watchdog("NodeGroup.DeactivateGroupInputDoesNotDeadlockWithRenderThread");
        // What a render thread does when it pulls an image through inputB: it gets the source, or nothing
        // while the inputs of the group are being changed.
        RenderThreadLoop renderThread([&dot, &source, &unexpectedInput]() {
            NodePtr input = dot->getInput(0);
            if ( input && (input != source) ) {
                unexpectedInput = true;
            }
        });

        // What the main thread does when the user deletes inputA (and undoes it).
        for (int i = 0; i < 2000; ++i) {
            inputA->deactivate(std::list<NodePtr>(), true /*disconnectAll*/, false /*reconnect*/, false /*hideGui*/,
                               false /*triggerRender*/, false /*unslaveKnobs*/);
            inputA->activate(std::list<NodePtr>(), true /*restoreAll*/, false /*triggerRender*/);
            connectGroupInputs();
        }
    }

    EXPECT_FALSE(unexpectedInput) << "The render thread got an input of the group that is not the source";
    EXPECT_EQ(2, groupNode->getNInputs());
    EXPECT_EQ( source, dot->getInput(0) );
    // Destroy the source first: removing a GroupInput loses the connections of the following inputs of the
    // group, which leaves stale outputs on the source (a separate, pre-existing bug), and destroying the source
    // after the group would then reach the destroyed group.
    source->destroyNode(true, false);
    groupNode->destroyNode(true, false);
}
