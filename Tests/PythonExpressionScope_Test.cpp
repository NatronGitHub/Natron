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

// Setting and evaluating a parameter expression must not write into the __main__ namespace of the
// Python interpreter, which the user scripts share (Script Editor, -t scripts, startup scripts): the
// variables of the expression (ret, frame, thisNode, random, the expression's own variables...) would
// overwrite theirs (#1252). The expressions still see the global variables of __main__.

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <string>

#include <gtest/gtest.h>

#include "Global/PythonUtils.h"

#include "Engine/AppInstance.h"
#include "Engine/AppManager.h"
#include "Engine/CreateNodeArgs.h"
#include "Engine/KnobTypes.h"
#include "Engine/Node.h"
#include "Engine/Project.h"

#include "OfxTestPlugin.h"

NATRON_NAMESPACE_USING

namespace {
// the variables of __main__ that the expressions used to overwrite
const char* const kUserVariables[] = {
    "ret", "frame", "view", "thisNode", "thisParam", "thisGroup", "random", "dimension", "s", "expression0",
};

class PythonExpressionScopeTest
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

        // the variables of a user script
        std::string script;
        for (std::size_t i = 0; i < sizeof(kUserVariables) / sizeof(kUserVariables[0]); ++i) {
            script += std::string(kUserVariables[i]) + " = 'user " + kUserVariables[i] + "'\n";
        }
        script += "def natronTestTwice(x):\n    return 2 * x\n";
        std::string error;
        EXPECT_TRUE( NATRON_NAMESPACE::NATRON_PYTHON_NAMESPACE::interpretPythonScript(script, &error, 0) ) << error;
    }

    virtual void TearDown() OVERRIDE
    {
        if (_node) {
            _node->destroyNode(true /*blockingDestroy*/, false /*autoReconnect*/);
            _node.reset();
        }
        std::string script;
        for (std::size_t i = 0; i < sizeof(kUserVariables) / sizeof(kUserVariables[0]); ++i) {
            script += std::string("del ") + kUserVariables[i] + "\n";
        }
        script += "del natronTestTwice\n";
        std::string error;
        NATRON_NAMESPACE::NATRON_PYTHON_NAMESPACE::interpretPythonScript(script, &error, 0);
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

    // the user variables of __main__ were not changed
    static void expectUserVariablesUnchanged()
    {
        PythonGILLocker pgl;
        PyObject* mainModule = NATRON_NAMESPACE::NATRON_PYTHON_NAMESPACE::getMainModule();

        for (std::size_t i = 0; i < sizeof(kUserVariables) / sizeof(kUserVariables[0]); ++i) {
            PyObject* value = PyObject_GetAttrString(mainModule, kUserVariables[i]); // new ref
            if (!value) {
                PyErr_Clear();
                ADD_FAILURE() << kUserVariables[i] << " was removed from __main__";
                continue;
            }
            std::string str = PyUnicode_Check(value) ? NATRON_NAMESPACE::NATRON_PYTHON_NAMESPACE::PyStringToStdString(value) : std::string("(not a string)");
            Py_DECREF(value);
            EXPECT_EQ(std::string("user ") + kUserVariables[i], str) << kUserVariables[i] << " was overwritten in __main__";
        }
    }
};
} // anonymous namespace

TEST_F(PythonExpressionScopeTest, ExpressionDoesNotWriteIntoMain)
{
    if ( skipped() ) {
        return;
    }
    KnobDoublePtr from = knob("doubleFrom");
    KnobDoublePtr to = knob("doubleTo");
    ASSERT_TRUE(from && to);
    from->setValue(5.);

    to->setExpression(0, "s = thisNode.doubleFrom.get() * 2\nret = s + frame + random(0, 0.1) * 0", true /*hasRetVariable*/, false /*failIfInvalid*/);
    ASSERT_FALSE( to->getExpression(0).empty() );
    std::string error;
    EXPECT_TRUE( to->isExpressionValid(0, &error) ) << error;
    expectUserVariablesUnchanged();

    EXPECT_EQ( 13., to->getValueAtTime(3.) );
    expectUserVariablesUnchanged();

    // the dependency on doubleFrom is still registered: a change of doubleFrom gives a new result
    from->setValue(6.);
    EXPECT_EQ( 15., to->getValueAtTime(3.) );
    expectUserVariablesUnchanged();

    to->clearExpression(0, true);
}

TEST_F(PythonExpressionScopeTest, SingleLineExpressionDoesNotWriteIntoMain)
{
    if ( skipped() ) {
        return;
    }
    KnobDoublePtr from = knob("doubleFrom");
    KnobDoublePtr to = knob("doubleTo");
    ASSERT_TRUE(from && to);
    from->setValue(2.);

    to->setExpression(0, "thisNode.doubleFrom.get() + frame", false /*hasRetVariable*/, false /*failIfInvalid*/);
    ASSERT_FALSE( to->getExpression(0).empty() );
    EXPECT_EQ( 6., to->getValueAtTime(4.) );
    expectUserVariablesUnchanged();

    to->clearExpression(0, true);
}

// The expressions still see the globals of __main__ (here a function of a user script), and their own
// variables in nested scopes (comprehensions, lambdas)
TEST_F(PythonExpressionScopeTest, ExpressionSeesGlobalsAndItsOwnVariables)
{
    if ( skipped() ) {
        return;
    }
    KnobDoublePtr from = knob("doubleFrom");
    KnobDoublePtr to = knob("doubleTo");
    ASSERT_TRUE(from && to);
    from->setValue(3.);

    to->setExpression(0, "natronTestTwice(frame)", false /*hasRetVariable*/, false /*failIfInvalid*/);
    std::string error;
    EXPECT_TRUE( to->isExpressionValid(0, &error) ) << error;
    EXPECT_EQ( 10., to->getValueAtTime(5.) );

    to->setExpression(0, "v = thisNode.doubleFrom.get()\nf = lambda x: x + v\nret = sum([f(i) for i in range(2)]) + sum([thisParam.curve(frame) * 0 + v for i in range(2)])",
                      true /*hasRetVariable*/, false /*failIfInvalid*/);
    ASSERT_FALSE( to->getExpression(0).empty() );
    EXPECT_TRUE( to->isExpressionValid(0, &error) ) << error;
    // (0 + 3) + (1 + 3) + 3 + 3
    EXPECT_EQ( 13., to->getValueAtTime(1.) );

    expectUserVariablesUnchanged();

    to->clearExpression(0, true);
}
