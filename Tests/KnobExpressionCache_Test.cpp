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

// The cache of the results of the Python expressions of a parameter (Knob<T>::_exprRes): a result computed
// while a parameter used by the expression changed, e.g. in the GUI or in another thread during a render,
// must not be stored, else it is used until the next change (#1309).

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <atomic>
#include <list>
#include <set>
#include <string>
#include <thread>
#include <utility>

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
class KnobExpressionCacheTest
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
};
} // anonymous namespace

// The expression reads doubleFrom, then changes it, as another thread could do during the evaluation.
TEST_F(KnobExpressionCacheTest, ResultIsNotStoredIfAnInputChangedDuringTheEvaluation)
{
    if ( skipped() ) {
        return;
    }
    KnobDoublePtr from = knob("doubleFrom");
    KnobDoublePtr to = knob("doubleTo");
    ASSERT_TRUE(from && to);
    from->setValue(10.);
    to->setExpression(0, "v = thisNode.doubleFrom.get()\nthisNode.doubleFrom.set(v + 1)\nret = v", true /*hasRetVariable*/, false /*failIfInvalid*/);
    ASSERT_FALSE( to->getExpression(0).empty() );

    // Evaluated with the value of doubleFrom when the evaluation started
    double v = from->getValue();
    EXPECT_EQ( v, to->getValueAtTime(1.) );
    EXPECT_EQ( v + 1, from->getValue() );

    // doubleFrom changed during that evaluation: its result must not be used again
    EXPECT_EQ( v + 1, to->getValueAtTime(1.) ) << "a result computed from an old value of doubleFrom was stored";

    to->clearExpression(0, true);
}

// Without changes, a result is computed once per frame (the cache still works)
TEST_F(KnobExpressionCacheTest, ResultIsStored)
{
    if ( skipped() ) {
        return;
    }
    KnobDoublePtr from = knob("doubleFrom");
    KnobDoublePtr to = knob("doubleTo");
    ASSERT_TRUE(from && to);
    from->setValue(3.);
    to->setExpression(0, "thisNode.doubleFrom.get() * 2 + frame", false /*hasRetVariable*/, false /*failIfInvalid*/);
    ASSERT_FALSE( to->getExpression(0).empty() );
    EXPECT_EQ( 7., to->getValueAtTime(1.) );
    EXPECT_EQ( 8., to->getValueAtTime(2.) );
    from->setValue(4.);
    // the results were cleared by the change of doubleFrom
    EXPECT_EQ( 9., to->getValueAtTime(1.) );

    to->clearExpression(0, true);
}

// #1544: clearExpression() modified the dependencies of the expression under mastersMutex, while
// getExpressionDependencies() and getAllExpressionDependenciesRecursive() (e.g. from a render thread) read
// them under expressionMutex: a data race on the same std::list. Here the main thread sets and clears the
// expression while another thread reads its dependencies. Run under ThreadSanitizer, the old code reports
// a data race; without it, it may crash.
TEST_F(KnobExpressionCacheTest, DependenciesAreReadWhileTheExpressionIsCleared)
{
    if ( skipped() ) {
        return;
    }
    KnobDoublePtr from = knob("doubleFrom");
    KnobDoublePtr to = knob("doubleTo");
    ASSERT_TRUE(from && to);

    std::atomic<bool> stop(false);
    std::atomic<int> nonEmptyReads(0);
    std::thread reader([&] {
        while (!stop) {
            std::list<std::pair<KnobIWPtr, int> > deps;
            if ( to->getExpressionDependencies(0, deps) && !deps.empty() ) {
                ++nonEmptyReads;
            }
            std::set<NodePtr> nodes;
            to->getAllExpressionDependenciesRecursive(nodes);
        }
    });
    for (int i = 0; i < 300; ++i) {
        to->setExpression(0, "thisNode.doubleFrom.get() + 1", false /*hasRetVariable*/, false /*failIfInvalid*/);
        to->clearExpression(0, true);
    }
    stop = true;
    reader.join();

    // the dependencies were all removed with the expression
    std::list<std::pair<KnobIWPtr, int> > deps;
    EXPECT_FALSE( to->getExpressionDependencies(0, deps) );
    to->setExpression(0, "thisNode.doubleFrom.get() + 1", false, false);
    ASSERT_TRUE( to->getExpressionDependencies(0, deps) );
    EXPECT_FALSE( deps.empty() );
    to->clearExpression(0, true);
}
