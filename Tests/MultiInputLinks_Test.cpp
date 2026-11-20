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

// Regression tests for https://github.com/NatronGitHub/Natron/issues/1168
// ("Undo and node edits lose a connection when a node feeds several inputs of the same node").
//
// Node::getOutputsConnectedToThisNode() must list every link: when a node feeds several inputs of the
// same node (e.g. a Read connected to both A and B of a Merge), one entry per input. Inserting a node
// after the selected node (NodeCollection::autoConnectNodes(), and its GUI counterpart) and inlining a
// Group rely on it.

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <iostream>
#include <list>
#include <utility>

#include <gtest/gtest.h>

#include "Engine/AppInstance.h"
#include "Engine/AppManager.h"
#include "Engine/CreateNodeArgs.h"
#include "Engine/Node.h"
#include "Engine/Plugin.h"
#include "Engine/Project.h"

NATRON_NAMESPACE_USING

namespace {
bool
isPluginAvailable(const char* pluginID)
{
    try {
        Plugin* p = appPTR->getPluginBinary(QString::fromUtf8(pluginID), -1, -1, false);

        return p && p->getLibraryBinary();
    } catch (...) {
        return false;
    }
}

class MultiInputLinksTest
    : public testing::Test
{
protected:

    virtual void SetUp() OVERRIDE FINAL
    {
        _app = appPTR->getTopLevelInstance();
        // A node with several inputs: Merge (B, A, Mask), from openfx-misc
        _havePlugins = isPluginAvailable(PLUGINID_OFX_MERGE);
        if (!_havePlugins) {
            // The bundled google-test predates GTEST_SKIP(): say clearly that the test did not run.
            // TODO: Convert to GTEST_SKIP() when gtest updated.
            const testing::TestInfo* info = testing::UnitTest::GetInstance()->current_test_info();
            std::cerr << "[  SKIPPED ] " << info->test_case_name() << '.' << info->name()
                      << ": plug-in " PLUGINID_OFX_MERGE " not found (needs openfx-misc in OFX_PLUGIN_PATH)" << std::endl;

            return;
        }
        _source = createNode(PLUGINID_NATRON_DOT);
        _merge = createNode(PLUGINID_OFX_MERGE);
        ASSERT_TRUE( bool(_source) );
        ASSERT_TRUE( bool(_merge) );
        ASSERT_GE(_merge->getNInputs(), 2);
        // _source feeds inputs 0 and 1 of _merge
        ASSERT_TRUE( _merge->connectInput(_source, 0) );
        ASSERT_TRUE( _merge->connectInput(_source, 1) );
    }

    virtual void TearDown() OVERRIDE FINAL
    {
        std::list<NodePtr> nodes;
        nodes.push_back(_inserted);
        nodes.push_back(_merge);
        nodes.push_back(_source);
        for (std::list<NodePtr>::iterator it = nodes.begin(); it != nodes.end(); ++it) {
            if (*it) {
                (*it)->destroyNode(true /*blockingDestroy*/, false /*autoReconnect*/);
            }
        }
        _inserted.reset();
        _merge.reset();
        _source.reset();
    }

    NodePtr createNode(const char* pluginID)
    {
        CreateNodeArgs args( pluginID, _app->getProject() );

        args.setProperty<bool>(kCreateNodeArgsPropAutoConnect, false);
        args.setProperty<bool>(kCreateNodeArgsPropAddUndoRedoCommand, false);

        return _app->createNode(args);
    }

    AppInstancePtr _app;
    bool _havePlugins;
    NodePtr _source, _merge, _inserted;
};
} // anonymous namespace

TEST_F(MultiInputLinksTest, OutputsConnectedToThisNodeListsEveryInput)
{
    if (!_havePlugins) {
        return;
    }
    std::list<std::pair<NodePtr, int> > outputs;
    _source->getOutputsConnectedToThisNode(&outputs);

    std::list<std::pair<NodePtr, int> > expected;
    expected.push_back( std::make_pair(_merge, 0) );
    expected.push_back( std::make_pair(_merge, 1) );
    EXPECT_EQ(expected, outputs);
}

// Creating a node while another one is selected inserts it after the selected node: every input that was
// connected to the selected node must now be connected to the inserted node.
TEST_F(MultiInputLinksTest, InsertNodeAfterNodeFeedingSeveralInputs)
{
    if (!_havePlugins) {
        return;
    }
    _inserted = createNode(PLUGINID_NATRON_DOT);
    ASSERT_TRUE( bool(_inserted) );

    EXPECT_TRUE( _app->getProject()->autoConnectNodes(_source, _inserted) );

    EXPECT_EQ( _source, _inserted->getRealInput(0) );
    EXPECT_EQ( _inserted, _merge->getRealInput(0) );
    EXPECT_EQ( _inserted, _merge->getRealInput(1) );
}
