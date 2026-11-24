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

// The random functions of the parameters, as documented in Documentation/source/devel/paramExpressions.rst
// and guide/compositing-exprs.rst (#1338):
// - random(min, max, frame, seed) only depends on the frame, the seed and the state of the node;
// - random(min, max) continues the sequence of the parameter. The evaluation of an expression of a
//   parameter starts the sequence of that parameter again from the frame and the dimension, so in its
//   own expression it is the same each time a frame is computed; elsewhere (in a script, or on another
//   parameter), it depends on the calls made before.

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
class ParamRandomTest
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

    KnobDoublePtr knob(const std::string& name) const
    {
        return std::dynamic_pointer_cast<KnobDouble>( _node->getKnobByName(name) );
    }

    // evaluate the expression again (not the stored result)
    static double evaluate(const KnobDoublePtr& k,
                           double frame,
                           int dimension = 0)
    {
        std::static_pointer_cast<KnobI>(k)->clearExpressionsResults(dimension); // public in KnobI

        return k->getValueAtTime(frame, dimension);
    }
};
} // anonymous namespace

// In the expression of the parameter itself, random(min, max) is the same each time a frame is
// computed, even after other calls, and differs between frames and dimensions
TEST_F(ParamRandomTest, RandomInItsOwnExpressionDependsOnTheFrame)
{
    if ( skipped() ) {
        return;
    }
    KnobDoublePtr to = knob("double2DTo");
    ASSERT_TRUE( bool(to) );
    for (int d = 0; d < 2; ++d) {
        to->setExpression(d, "random(0, 100) + random(0, 100) * 0", false /*hasRetVariable*/, false /*failIfInvalid*/);
        ASSERT_FALSE( to->getExpression(d).empty() );
    }
    const double v1 = evaluate(to, 1.);
    const double v2 = evaluate(to, 2.);
    EXPECT_NE(v1, v2);
    EXPECT_NE( v1, evaluate(to, 1., 1) );
    // other calls in between
    double r = to->random(0., 1.) + to->random(0., 1.) + evaluate(to, 7.);
    (void)r;
    EXPECT_EQ( v1, evaluate(to, 1.) );
    EXPECT_EQ( v2, evaluate(to, 2.) );

    for (int d = 0; d < 2; ++d) {
        to->clearExpression(d, true);
    }
}

// random(min, max, frame, seed) only depends on the frame and the seed (for a given node state)
TEST_F(ParamRandomTest, RandomWithFrameAndSeed)
{
    if ( skipped() ) {
        return;
    }
    KnobDoublePtr to = knob("doubleTo");
    ASSERT_TRUE( bool(to) );
    const double a = to->random(0., 1., 5., 0);
    EXPECT_NE( a, to->random(0., 1., 5., 1) );
    EXPECT_NE( a, to->random(0., 1., 6., 0) );
    double r = to->random(0., 1.);
    (void)r;
    EXPECT_EQ( a, to->random(0., 1., 5., 0) );
    // it does not depend on the parameter (as documented: use a different seed per parameter and dimension)
    KnobDoublePtr from = knob("doubleFrom");
    ASSERT_TRUE( bool(from) );
    EXPECT_EQ( a, from->random(0., 1., 5., 0) );

    // the same in an expression
    to->setExpression(0, "random(0, 1, frame, 0)", false /*hasRetVariable*/, false /*failIfInvalid*/);
    ASSERT_FALSE( to->getExpression(0).empty() );
    EXPECT_EQ( a, evaluate(to, 5.) );
    to->clearExpression(0, true);
}

// Outside the expression of the parameter, random(min, max) continues the sequence: two calls differ
TEST_F(ParamRandomTest, RandomOutsideAnExpressionDependsOnThePreviousCalls)
{
    if ( skipped() ) {
        return;
    }
    KnobDoublePtr to = knob("doubleTo");
    ASSERT_TRUE( bool(to) );
    const double a = to->random(0., 1.);
    EXPECT_NE( a, to->random(0., 1.) );
}
