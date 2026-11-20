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

// The input that a new node connects to the selected node (Node::getPreferredInputForConnection()):
// Shuffle (version 3 and later, which outputs B by default) connects B, whatever the preference
// "Merge node connect to A input" (#818), which still applies to Merge.
// Needs the Shuffle and Merge plug-ins of openfx-misc (in OFX_PLUGIN_PATH); skipped otherwise.

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <iostream>
#include <string>

#include <gtest/gtest.h>

#include <QtCore/QString>

#include "Engine/AppInstance.h"
#include "Engine/AppManager.h"
#include "Engine/CreateNodeArgs.h"
#include "Engine/EffectInstance.h"
#include "Engine/KnobTypes.h"
#include "Engine/Node.h"
#include "Engine/Plugin.h"
#include "Engine/Project.h"
#include "Engine/Settings.h"

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

// The bundled google-test predates GTEST_SKIP(): the tests return early and are reported as passed,
// so say clearly that they did not run.
void
reportSkipped(const std::string& why)
{
    const testing::TestInfo* info = testing::UnitTest::GetInstance()->current_test_info();

    std::cerr << "[  SKIPPED ] " << info->test_case_name() << '.' << info->name() << ": " << why << std::endl;
}

class ShuffleAutoConnectTest
    : public testing::Test
{
protected:
    KnobBoolPtr _connectToA;
    bool _connectToAWas;

    ShuffleAutoConnectTest()
        : _connectToA()
        , _connectToAWas(true)
    {
    }

    virtual void SetUp() OVERRIDE
    {
        _connectToA = std::dynamic_pointer_cast<KnobBool>( appPTR->getCurrentSettings()->getKnobByName("mergeConnectToA") );
        if (_connectToA) {
            _connectToAWas = _connectToA->getValue();
        }
    }

    virtual void TearDown() OVERRIDE
    {
        if (_connectToA) {
            _connectToA->setValue(_connectToAWas);
        }
    }

    // the label of the input that the node created from pluginID connects to the selected node
    std::string preferredInputLabel(const char* pluginID)
    {
        AppInstancePtr app = appPTR->getTopLevelInstance();
        CreateNodeArgs args( pluginID, app->getProject() );

        args.setProperty<bool>(kCreateNodeArgsPropAddUndoRedoCommand, false);
        NodePtr node = app->createNode(args);
        if (!node) {
            return "(not created)";
        }
        int i = node->getPreferredInputForConnection();
        std::string label = (i >= 0) ? node->getInputLabel(i) : std::string("(none)");
        node->destroyNode(true /*blockingDestroy*/, false /*autoReconnect*/);

        return label;
    }
};
} // anonymous namespace

TEST_F(ShuffleAutoConnectTest, ShuffleConnectsB)
{
    if ( !isPluginAvailable(PLUGINID_OFX_SHUFFLE) ) {
        reportSkipped("needs " PLUGINID_OFX_SHUFFLE " (openfx-misc) in OFX_PLUGIN_PATH");

        return;
    }
    ASSERT_TRUE( bool(_connectToA) );

    _connectToA->setValue(true); // the default
    EXPECT_EQ( "B", preferredInputLabel(PLUGINID_OFX_SHUFFLE) );
    _connectToA->setValue(false);
    EXPECT_EQ( "B", preferredInputLabel(PLUGINID_OFX_SHUFFLE) );
}

// the preference still applies to Merge
TEST_F(ShuffleAutoConnectTest, MergeFollowsThePreference)
{
    if ( !isPluginAvailable(PLUGINID_OFX_MERGE) ) {
        reportSkipped("needs " PLUGINID_OFX_MERGE " (openfx-misc) in OFX_PLUGIN_PATH");

        return;
    }
    ASSERT_TRUE( bool(_connectToA) );

    _connectToA->setValue(true);
    EXPECT_EQ( "A", preferredInputLabel(PLUGINID_OFX_MERGE) );
    _connectToA->setValue(false);
    EXPECT_EQ( "B", preferredInputLabel(PLUGINID_OFX_MERGE) );
}
