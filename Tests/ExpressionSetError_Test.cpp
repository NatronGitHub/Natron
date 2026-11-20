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

// Param.setExpression() from Python returns False for an invalid expression, and also prints why, in
// the Script Editor of the GUI or on the standard output of NatronRenderer (#1250).

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
class ExpressionSetErrorTest
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
        std::string error;
        NATRON_NAMESPACE::NATRON_PYTHON_NAMESPACE::interpretPythonScript("natronTestResult = None\n", &error, 0);
    }

    bool skipped()
    {
        if (!_node) {
            NatronTestPlugin::reportSkipped(kNatronTestPluginParams);

            return true;
        }

        return false;
    }

    // call setExpression() from Python on doubleTo; returns what it printed, and its result in *result
    std::string setExpression(const std::string& expr,
                              bool* result)
    {
        std::string script = "natronTestResult = " + _node->getApp()->getAppIDString() + '.' + _node->getFullyQualifiedName() +
                             ".doubleTo.setExpression(" + expr + ", False)\n";
        std::string error;
        testing::internal::CaptureStdout();
        testing::internal::CaptureStderr();
        bool ok = NATRON_NAMESPACE::NATRON_PYTHON_NAMESPACE::interpretPythonScript(script, &error, 0);
        std::string errOutput = testing::internal::GetCapturedStderr();
        std::string output = testing::internal::GetCapturedStdout();
        // the exception is normalized before it is formatted (no secondary error from the traceback module)
        EXPECT_EQ( std::string::npos, errOutput.find("__suppress_context__") ) << errOutput;
        EXPECT_TRUE(ok) << "setExpression() must not raise: " << error;

        PythonGILLocker pgl;
        PyObject* ret = PyObject_GetAttrString(NATRON_NAMESPACE::NATRON_PYTHON_NAMESPACE::getMainModule(), "natronTestResult"); // new ref
        EXPECT_TRUE(ret != 0);
        *result = ret && PyObject_IsTrue(ret) == 1;
        Py_XDECREF(ret);
        PyErr_Clear();

        return output;
    }
};
} // anonymous namespace

TEST_F(ExpressionSetErrorTest, InvalidExpressionIsReported)
{
    if ( skipped() ) {
        return;
    }
    KnobDoublePtr to = std::dynamic_pointer_cast<KnobDouble>( _node->getKnobByName("doubleTo") );
    ASSERT_TRUE( bool(to) );

    bool result = true;
    std::string output = setExpression("'1/0'", &result);
    EXPECT_FALSE(result);
    EXPECT_TRUE( to->getExpression(0).empty() );
    EXPECT_NE( std::string::npos, output.find("invalid expression on") ) << output;
    EXPECT_NE( std::string::npos, output.find("doubleTo") ) << output;
    EXPECT_NE( std::string::npos, output.find("division by zero") ) << output;

    output = setExpression("'natronTestUnknownNode.x.get()'", &result);
    EXPECT_FALSE(result);
    EXPECT_NE( std::string::npos, output.find("natronTestUnknownNode") ) << output;

    output = setExpression("'1 +'", &result);
    EXPECT_FALSE(result);
    EXPECT_NE( std::string::npos, output.find("invalid expression on") ) << output;
    // only the Python error, not the generated script
    EXPECT_EQ( std::string::npos, output.find("def expression") ) << output;
}

// A failed setExpression() also removes the previous expression of that dimension (as documented)
TEST_F(ExpressionSetErrorTest, InvalidExpressionRemovesThePreviousOne)
{
    if ( skipped() ) {
        return;
    }
    KnobDoublePtr to = std::dynamic_pointer_cast<KnobDouble>( _node->getKnobByName("doubleTo") );
    ASSERT_TRUE( bool(to) );

    bool result = false;
    setExpression("'frame * 2'", &result);
    EXPECT_TRUE(result);
    EXPECT_FALSE( to->getExpression(0).empty() );
    setExpression("'1/0'", &result);
    EXPECT_FALSE(result);
    EXPECT_TRUE( to->getExpression(0).empty() );
}

TEST_F(ExpressionSetErrorTest, ValidExpressionIsSilent)
{
    if ( skipped() ) {
        return;
    }
    KnobDoublePtr to = std::dynamic_pointer_cast<KnobDouble>( _node->getKnobByName("doubleTo") );
    ASSERT_TRUE( bool(to) );

    bool result = false;
    std::string output = setExpression("'frame * 2'", &result);
    EXPECT_TRUE(result);
    EXPECT_EQ( std::string::npos, output.find("invalid expression") ) << output;
    EXPECT_EQ( 6., to->getValueAtTime(3.) );
    to->clearExpression(0, true);
}
