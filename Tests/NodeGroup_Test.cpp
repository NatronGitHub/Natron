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

// Regression test for a deadlock between the main thread and render threads in groups (related to
// https://github.com/NatronGitHub/Natron/issues/814, "Application freeze when copying a node with enabled
// postage stamp").
//
// Node::initializeInputs() (main thread) held the group node's inputsMutex while calling
// NodeGroup::isInputMask(), which takes the group's nodesLock. A render thread pulling an image through a
// GroupInput calls NodeGroup::getRealInputForInput(), which holds nodesLock while calling Node::getInput(),
// which takes inputsMutex. Each thread then waited for the lock held by the other one, freezing the GUI.

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <atomic>
#include <chrono>
#include <cstdlib>
#include <iostream>
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

    const int iterations = 20000;
    std::atomic<bool> done(false);

    // If the two threads deadlock, nothing else can report it: abort after a while.
    std::thread watchdog([&done]() {
        for (int i = 0; i < 600 && !done; ++i) {
            std::this_thread::sleep_for( std::chrono::milliseconds(100) );
        }
        if (!done) {
            std::cerr << "NodeGroup.InitializeInputsDoesNotDeadlockWithRenderThread: deadlock (no progress after 60 s)" << std::endl;
            std::abort();
        }
    });

    // What a render thread does when it pulls an image through the GroupInput: nodesLock, then inputsMutex.
    std::thread renderThread([&group, &inputNode, iterations]() {
        for (int i = 0; i < iterations; ++i) {
            (void)group->getRealInputForInput(false, inputNode);
        }
    });

    // What the main thread does when a GroupInput's mask/optional state or name changes.
    for (int i = 0; i < iterations; ++i) {
        groupNode->initializeInputs();
    }
    renderThread.join();
    done = true;
    watchdog.join();

    EXPECT_EQ(1, groupNode->getNInputs());

    groupNode->destroyNode(true, false);
}
