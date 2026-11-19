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

// The interpolation callback of a custom parameter (kOfxParamPropCustomInterpCallbackV1, OFX 1.5.1
// ofxParam.h, OfxCustomParamInterpFuncV1) gets "the plugin instance that this parameter occurs in", i.e. its
// parameter set, and the host uses its result only if it succeeds. The test plug-in net.natron.test.Params
// (Tests/OfxTestPlugin) checks the first argument of the callback of its custom parameters.

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <string>

#include <gtest/gtest.h>

#include "Engine/AppInstance.h"
#include "Engine/AppManager.h"
#include "Engine/CreateNodeArgs.h"
#include "Engine/KnobTypes.h"
#include "Engine/Node.h"
#include "Engine/Project.h"

#include "OfxTestPlugin.h"

NATRON_NAMESPACE_USING

namespace {
class OfxCustomParamInterpTest
    : public ::testing::Test
{
protected:
    NodePtr _node;
    KnobStringPtr _custom;

    virtual void SetUp() OVERRIDE
    {
        if ( !NatronTestPlugin::isAvailable(kNatronTestPluginParams) ) {
            return;
        }
        AppInstancePtr app = appPTR->getTopLevelInstance();
        CreateNodeArgs args( kNatronTestPluginParams, app->getProject() );
        args.setProperty<bool>(kCreateNodeArgsPropAddUndoRedoCommand, false);
        _node = app->createNode(args);
        if (_node) {
            _custom = std::dynamic_pointer_cast<KnobString>( _node->getKnobByName("customFrom") );
        }
    }

    virtual void TearDown() OVERRIDE
    {
        _custom.reset();
        if (_node) {
            _node->destroyNode(true /*blockingDestroy*/, false /*autoReconnect*/);
            _node.reset();
        }
    }

    bool skipped()
    {
        if (!_node) {
            NatronTestPlugin::reportSkipped(kNatronTestPluginParams);

            return true;
        }

        return false;
    }
};
} // anonymous namespace

// Between two keyframes, the value is the result of the callback, which gets the parameter set.
TEST_F(OfxCustomParamInterpTest, InterpolatesWithTheParamSet)
{
    if ( skipped() ) {
        return;
    }
    ASSERT_TRUE( bool(_custom) );
    const int ok = NatronTestPlugin::counter("Params.interp.ok");
    const int badHandle = NatronTestPlugin::counter("Params.interp.badHandle");

    _custom->setValueAtTime(1., "a", ViewSpec::all(), 0);
    _custom->setValueAtTime(5., "b", ViewSpec::all(), 0);
    EXPECT_EQ( "a", _custom->getValueAtTime(1., 0) );
    EXPECT_EQ( "b", _custom->getValueAtTime(5., 0) );
    EXPECT_EQ( "a>b@0.50", _custom->getValueAtTime(3., 0) );
    EXPECT_EQ( "a>b@0.25", _custom->getValueAtTime(2., 0) );
    EXPECT_EQ( badHandle, NatronTestPlugin::counter("Params.interp.badHandle") );
    EXPECT_EQ( ok + 2, NatronTestPlugin::counter("Params.interp.ok") );
}

// If the callback fails, the value is the one of the previous keyframe, not an empty string.
TEST_F(OfxCustomParamInterpTest, FailedCallbackGivesPreviousKey)
{
    if ( skipped() ) {
        return;
    }
    ASSERT_TRUE( bool(_custom) );
    const int failed = NatronTestPlugin::counter("Params.interp.failed");

    _custom->setValueAtTime(1., "fail", ViewSpec::all(), 0);
    _custom->setValueAtTime(5., "b", ViewSpec::all(), 0);
    EXPECT_EQ( "fail", _custom->getValueAtTime(3., 0) );
    EXPECT_EQ( "b", _custom->getValueAtTime(5., 0) );
    EXPECT_EQ( failed + 1, NatronTestPlugin::counter("Params.interp.failed") );
}
