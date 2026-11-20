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

// Regression test for https://github.com/NatronGitHub/Natron/issues/777: in Python, two Effect objects that refer
// to the same node (e.g. two calls to app.getNode()) were not equal, since each call creates a new Effect.
// They must be equal, and stay hashable (usable in sets and as dict keys), with equal hashes.

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <string>

#include <gtest/gtest.h>

#include "Engine/AppInstance.h"
#include "Engine/AppManager.h" // NATRON_NAMESPACE::NATRON_PYTHON_NAMESPACE::interpretPythonScript
#include "Engine/CreateNodeArgs.h"
#include "Engine/Node.h"
#include "Engine/Project.h"
#include "Engine/PyNode.h"

NATRON_NAMESPACE_USING

namespace {
NodePtr
createDot(const AppInstancePtr& app)
{
    CreateNodeArgs args( PLUGINID_NATRON_DOT, app->getProject() );

    args.setProperty<bool>(kCreateNodeArgsPropAddUndoRedoCommand, false);
    args.setProperty<bool>(kCreateNodeArgsPropAutoConnect, false);

    return app->createNode(args);
}
} // anonymous namespace

TEST(PyEffect, EqualityAndHash)
{
    AppInstancePtr app = appPTR->getTopLevelInstance();
    ASSERT_TRUE( bool(app) );
    NodePtr dot1 = createDot(app);
    NodePtr dot2 = createDot(app);
    ASSERT_TRUE( dot1 && dot2 );

    // C++
    {
        typedef NATRON_NAMESPACE::NATRON_PYTHON_NAMESPACE::Effect PyEffect;
        PyEffect a(dot1), aAgain(dot1), b(dot2);
        EXPECT_TRUE(a == aAgain);
        EXPECT_FALSE(a != aAgain);
        EXPECT_FALSE(a == b);
        EXPECT_TRUE(a != b);
        EXPECT_EQ( NATRON_NAMESPACE::NATRON_PYTHON_NAMESPACE::hashEffect(a), NATRON_NAMESPACE::NATRON_PYTHON_NAMESPACE::hashEffect(aAgain) );
        EXPECT_NE( NATRON_NAMESPACE::NATRON_PYTHON_NAMESPACE::hashEffect(a), NATRON_NAMESPACE::NATRON_PYTHON_NAMESPACE::hashEffect(b) );
    }

    // Python
    const std::string app_ = app->getAppIDString();
    const std::string script =
        "a = " + app_ + ".getNode('" + dot1->getScriptName() + "')\n"
        "aAgain = " + app_ + ".getNode('" + dot1->getScriptName() + "')\n"
        "b = " + app_ + ".getNode('" + dot2->getScriptName() + "')\n"
        "assert a is not aAgain\n"
        "assert a == aAgain\n"
        "assert not (a != aAgain)\n"
        "assert a != b\n"
        "assert not (a == b)\n"
        "assert not (a == 3)\n"
        "assert a != None\n"
        "assert hash(a) == hash(aAgain)\n"
        "assert len(set([a, aAgain, b])) == 2\n"
        "assert {a: 1}[aAgain] == 1\n"
        "assert a in " + app_ + ".getChildren()\n"
        "del a, aAgain, b\n";
    std::string err, output;
    bool ok = NATRON_NAMESPACE::NATRON_PYTHON_NAMESPACE::interpretPythonScript(script, &err, &output);
    EXPECT_TRUE(ok) << script << err;
    EXPECT_EQ(std::string(), err) << script;

    dot1->destroyNode(true, false);
    dot2->destroyNode(true, false);
}
