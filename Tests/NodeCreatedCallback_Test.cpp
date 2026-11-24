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

// Regression tests for https://github.com/NatronGitHub/Natron/issues/488: the project's afterNodeCreated callback
// (and beforeNodeRemoval) must be called once per node, also for a Read node, and a Group's own callbacks once for
// each node created in (or removed from) that Group.

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <list>
#include <string>

#include <gtest/gtest.h>

#include "Engine/AppInstance.h"
#include "Engine/AppManager.h"
#include "Engine/CreateNodeArgs.h"
#include "Engine/EffectInstance.h"
#include "Engine/KnobTypes.h"
#include "Engine/Node.h"
#include "Engine/NodeGroup.h"
#include "Engine/Project.h"

#include "BaseTest.h"

NATRON_NAMESPACE_USING

namespace {
// Python callbacks that record the script name of each node they are called for, in the lists
// natronTest488<Name> of __main__.
const char* const kDefineCallbacks =
    "natronTest488Created = []\n"
    "natronTest488Removed = []\n"
    "natronTest488GroupCreated = []\n"
    "natronTest488GroupRemoved = []\n"
    "def natronTest488OnCreated(thisNode, app, userEdited):\n"
    "    natronTest488Created.append(thisNode.getScriptName())\n"
    "def natronTest488OnRemoved(thisNode, app):\n"
    "    natronTest488Removed.append(thisNode.getScriptName())\n"
    "def natronTest488OnGroupCreated(thisNode, app, userEdited):\n"
    "    natronTest488GroupCreated.append(thisNode.getScriptName())\n"
    "def natronTest488OnGroupRemoved(thisNode, app):\n"
    "    natronTest488GroupRemoved.append(thisNode.getScriptName())\n";

void
runPython(const std::string& script)
{
    std::string error;

    EXPECT_TRUE( NATRON_NAMESPACE::NATRON_PYTHON_NAMESPACE::interpretPythonScript(script, &error, 0) ) << error;
}

// How many times the callback that records into the list natronTest488<name> was called for scriptName
int
callCount(const std::string& name,
          const std::string& scriptName)
{
    runPython("natronTestCount = natronTest488" + name + ".count('" + scriptName + "')\n");
    PythonGILLocker pgl;
    PyObject* count = PyObject_GetAttrString(NATRON_NAMESPACE::NATRON_PYTHON_NAMESPACE::getMainModule(), "natronTestCount"); // new ref
    EXPECT_TRUE(count != 0);
    int ret = count ? (int)PyLong_AsLong(count) : -1;
    Py_XDECREF(count);
    PyErr_Clear();

    return ret;
}

void
setStringKnob(const KnobHolder* holder,
              const std::string& name,
              const std::string& value)
{
    KnobStringPtr knob = std::dynamic_pointer_cast<KnobString>( holder->getKnobByName(name) );

    ASSERT_TRUE( bool(knob) ) << name;
    knob->setValue(value);
}

// Sets the project's afterNodeCreated and beforeNodeRemoval callbacks for the life of the object.
class ProjectCallbacks
{
    ProjectPtr _project;

public:
    explicit ProjectCallbacks(const ProjectPtr& project)
        : _project(project)
    {
        runPython(kDefineCallbacks);
        setStringKnob(_project.get(), "afterNodeCreated", "natronTest488OnCreated");
        setStringKnob(_project.get(), "beforeNodeRemoval", "natronTest488OnRemoved");
    }

    ~ProjectCallbacks()
    {
        setStringKnob(_project.get(), "afterNodeCreated", "");
        setStringKnob(_project.get(), "beforeNodeRemoval", "");
        runPython("for natronTestName in [n for n in globals() if n.startswith('natronTest488')]:\n"
                  "    del globals()[natronTestName]\n"
                  "globals().pop('natronTestName', None)\n"
                  "natronTestCount = None\n");
    }
};

NodePtr
createNodeIn(const AppInstancePtr& app,
             const std::string& pluginID,
             const NodeCollectionPtr& group)
{
    CreateNodeArgs args(pluginID, group);

    args.setProperty<bool>(kCreateNodeArgsPropNodeGroupDisableCreateInitialNodes, true);
    args.setProperty<bool>(kCreateNodeArgsPropAddUndoRedoCommand, false);
    args.setProperty<bool>(kCreateNodeArgsPropAutoConnect, false);
    args.setProperty<bool>(kCreateNodeArgsPropSilent, true);

    return app->createNode(args);
}

// Deletes the node as the GUI does (deactivate(), which runs beforeNodeRemoval), then destroys it.
void
deleteNode(const NodePtr& node)
{
    node->deactivate(std::list<NodePtr>(), true /*disconnectAll*/, false /*reconnect*/, true /*hideGui*/, false /*triggerRender*/);
    node->destroyNode(true /*blockingDestroy*/, false /*autoReconnect*/);
}
} // anonymous namespace

// A node at the top level of the project: the project's callbacks are called once.
TEST(NodeCreatedCallback, CalledOnceForTopLevelNode)
{
    AppInstancePtr app = appPTR->getTopLevelInstance();
    ASSERT_TRUE( bool(app) );
    ProjectCallbacks callbacks( app->getProject() );

    NodePtr dot = createNodeIn(app, PLUGINID_NATRON_DOT, app->getProject() );
    ASSERT_TRUE( bool(dot) );
    const std::string name = dot->getScriptName();
    EXPECT_EQ( 1, callCount("Created", name) );

    deleteNode(dot);
    EXPECT_EQ( 1, callCount("Removed", name) );
}

// A node in a Group: the project's callbacks and the Group's own callbacks are each called once.
TEST(NodeCreatedCallback, CalledOnceForNodeInGroup)
{
    AppInstancePtr app = appPTR->getTopLevelInstance();
    ASSERT_TRUE( bool(app) );
    ProjectCallbacks callbacks( app->getProject() );

    NodePtr groupNode = createNodeIn(app, PLUGINID_NATRON_GROUP, app->getProject() );
    ASSERT_TRUE( bool(groupNode) );
    EXPECT_EQ( 1, callCount("Created", groupNode->getScriptName() ) );
    NodeGroupPtr group = std::dynamic_pointer_cast<NodeGroup>( groupNode->getEffectInstance() );
    ASSERT_TRUE( bool(group) );
    setStringKnob(group.get(), "afterNodeCreated", "natronTest488OnGroupCreated");
    setStringKnob(group.get(), "beforeNodeRemoval", "natronTest488OnGroupRemoved");

    NodePtr dot = createNodeIn(app, PLUGINID_NATRON_DOT, group);
    ASSERT_TRUE( bool(dot) );
    const std::string name = dot->getScriptName();
    EXPECT_EQ( 1, callCount("Created", name) );
    EXPECT_EQ( 1, callCount("GroupCreated", name) );

    deleteNode(dot);
    EXPECT_EQ( 1, callCount("Removed", name) );
    EXPECT_EQ( 1, callCount("GroupRemoved", name) );

    groupNode->destroyNode(true /*blockingDestroy*/, false /*autoReconnect*/);
}

// The Read node of the issue (it needs the ReadOIIO plug-in, as all the BaseTest tests).
TEST_F(BaseTest, NodeCreatedCallbackCalledOnceForReadNode)
{
    ProjectCallbacks callbacks( getApp()->getProject() );

    NodePtr reader = createNodeIn(getApp(), PLUGINID_NATRON_READ, getApp()->getProject() );
    ASSERT_TRUE( bool(reader) );
    const std::string name = reader->getScriptName();
    EXPECT_EQ( 1, callCount("Created", name) );

    deleteNode(reader);
    EXPECT_EQ( 1, callCount("Removed", name) );
}
