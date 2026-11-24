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

// The app keeps the list of knobs with an invalid expression, which it re-checks when nodes
// are created, renamed or connected. A knob must stay in it while at least one of its
// dimensions has an invalid expression, and every such knob must be in it.

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
class KnobInvalidExpressionTest
    : public ::testing::Test
{
protected:
    NodePtr _node;

    virtual void SetUp() OVERRIDE
    {
        if ( !NatronTestPlugin::isAvailable(kNatronTestPluginParams) ) {
            return;
        }
        AppInstancePtr app = appPTR->getTopLevelInstance();
        CreateNodeArgs args( kNatronTestPluginParams, app->getProject() );
        args.setProperty<bool>(kCreateNodeArgsPropAddUndoRedoCommand, false);
        _node = app->createNode(args);
    }

    virtual void TearDown() OVERRIDE
    {
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

    KnobIPtr knob(const std::string& name) const
    {
        return _node->getKnobByName(name);
    }

    bool isListed(const KnobIPtr& k) const
    {
        return _node->getApp()->isInvalidExpressionKnob( k.get() );
    }
};
} // anonymous namespace

// Making one dimension valid again must not drop the knob while another dimension is still invalid.
TEST_F(KnobInvalidExpressionTest, KnobStaysListedWhileAnotherDimensionIsInvalid)
{
    if ( skipped() ) {
        return;
    }
    KnobIPtr k = knob("double2DFrom");
    ASSERT_TRUE(k != nullptr);
    ASSERT_EQ( 2, k->getDimension() );

    k->setExpressionInvalid(0, false, "error in x");
    k->setExpressionInvalid(1, false, "error in y");
    EXPECT_TRUE( isListed(k) );

    k->setExpressionInvalid(0, true, std::string());
    EXPECT_TRUE( isListed(k) ) << "y is still invalid";

    k->setExpressionInvalid(1, true, std::string());
    EXPECT_FALSE( isListed(k) );
}

// Every knob with an invalid expression is listed, not only the first one.
TEST_F(KnobInvalidExpressionTest, EveryInvalidKnobIsListed)
{
    if ( skipped() ) {
        return;
    }
    KnobIPtr a = knob("double2DFrom");
    KnobIPtr b = knob("rgbFrom");
    ASSERT_TRUE(a && b);

    a->setExpressionInvalid(0, false, "error in a");
    b->setExpressionInvalid(0, false, "error in b");
    EXPECT_TRUE( isListed(a) );
    EXPECT_TRUE( isListed(b) ) << "only the first knob with an invalid expression was listed";

    a->setExpressionInvalid(0, true, std::string());
    b->setExpressionInvalid(0, true, std::string());
    EXPECT_FALSE( isListed(a) );
    EXPECT_FALSE( isListed(b) );
}

// A knob whose expression becomes invalid while recheckInvalidExpressions() runs stays listed (#1542):
// re-checking doubleTo evaluates the expression of doubleFrom, which fails.
TEST_F(KnobInvalidExpressionTest, KnobInvalidatedDuringRecheckStaysListed)
{
    if ( skipped() ) {
        return;
    }
    KnobIPtr from = knob("doubleFrom");
    KnobIPtr to = knob("doubleTo");
    ASSERT_TRUE(from && to);

    std::string error;
    ASSERT_TRUE( NATRON_NAMESPACE::NATRON_PYTHON_NAMESPACE::interpretPythonScript("natronTestBroken = False\nnatronTestFrame = 1\n", &error, 0) ) << error;
    from->setExpression(0, "natronTestMissing if natronTestBroken else 1.", false, false);
    EXPECT_TRUE( from->isExpressionValid(0, 0) );
    // always invalid (NameError), after reading doubleFrom at a frame that is not in its cache yet
    to->setExpression(0, "thisNode.doubleFrom.getValueAtTime(natronTestFrame) + natronTestMissing", false, false);
    EXPECT_TRUE( isListed(to) );
    EXPECT_FALSE( isListed(from) );

    ASSERT_TRUE( NATRON_NAMESPACE::NATRON_PYTHON_NAMESPACE::interpretPythonScript("natronTestBroken = True\nnatronTestFrame = 2\n", &error, 0) ) << error;
    _node->getApp()->recheckInvalidExpressions();
    EXPECT_FALSE( from->isExpressionValid(0, 0) ) << "the re-check did not evaluate doubleFrom";
    EXPECT_TRUE( isListed(to) );
    EXPECT_TRUE( isListed(from) ) << "the knob listed during the re-check was dropped";

    NATRON_NAMESPACE::NATRON_PYTHON_NAMESPACE::interpretPythonScript("del natronTestBroken, natronTestFrame\n", &error, 0);
}
