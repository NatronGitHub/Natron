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

// kOfxImageEffectPluginPropSingleInstance (ofxImageEffect.h): "1 - which means only one instance can
// exist at any one time". Natron never read it (#1364).

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <gtest/gtest.h>

#include <QtCore/QTemporaryDir>

#include "Engine/AppInstance.h"
#include "Engine/AppManager.h"
#include "Engine/CreateNodeArgs.h"
#include "Engine/Node.h"
#include "Engine/Project.h"

#include "OfxTestPlugin.h"

NATRON_NAMESPACE_USING

namespace {
NodePtr
createSingleInstanceNode(bool outOfProject = false)
{
    AppInstancePtr app = appPTR->getTopLevelInstance();
    CreateNodeArgs args( kNatronTestPluginSingleInstance, app->getProject() );

    args.setProperty<bool>(kCreateNodeArgsPropAddUndoRedoCommand, false);
    args.setProperty<bool>(kCreateNodeArgsPropSilent, true); // no error dialog
    if (outOfProject) {
        args.setProperty<bool>(kCreateNodeArgsPropOutOfProject, true);
        args.setProperty<bool>(kCreateNodeArgsPropNoNodeGUI, true);
    }

    return app->createNode(args);
}

void
destroy(NodePtr& node)
{
    if (node) {
        node->destroyNode(true /*blockingDestroy*/, false /*autoReconnect*/);
        node.reset();
    }
}
}

TEST(OfxSingleInstance, OnlyOneInstance)
{
    if ( !NatronTestPlugin::isAvailable(kNatronTestPluginSingleInstance) ) {
        NatronTestPlugin::reportSkipped(kNatronTestPluginSingleInstance);

        return;
    }
    ASSERT_EQ( 0, NatronTestPlugin::counter("SingleInstance.live") );

    NodePtr first = createSingleInstanceNode();
    ASSERT_TRUE( bool(first) );
    EXPECT_EQ( 1, NatronTestPlugin::counter("SingleInstance.live") );

    // a second node is refused, and the plug-in never sees a second instance
    NodePtr second = createSingleInstanceNode();
    EXPECT_FALSE( bool(second) );
    EXPECT_EQ( 1, NatronTestPlugin::counter("SingleInstance.live") );
    destroy(second);

    // once the first one is gone, a new one may be created
    first->destroyNode(true /*blockingDestroy*/, false /*autoReconnect*/);
    first.reset();
    EXPECT_EQ( 0, NatronTestPlugin::counter("SingleInstance.live") );
    NodePtr third = createSingleInstanceNode();
    EXPECT_TRUE( bool(third) );
    EXPECT_EQ( 1, NatronTestPlugin::counter("SingleInstance.live") );
    destroy(third);
    EXPECT_EQ( 0, NatronTestPlugin::counter("SingleInstance.live") );
}

// A temporary node out of the project (DocumentationManager, to show the plug-in's help) is allowed.
TEST(OfxSingleInstance, OutOfProjectNode)
{
    if ( !NatronTestPlugin::isAvailable(kNatronTestPluginSingleInstance) ) {
        NatronTestPlugin::reportSkipped(kNatronTestPluginSingleInstance);

        return;
    }
    NodePtr first = createSingleInstanceNode();
    ASSERT_TRUE( bool(first) );
    NodePtr doc = createSingleInstanceNode(true /*outOfProject*/);
    EXPECT_TRUE( bool(doc) );
    destroy(doc);
    // and it doesn't count either
    destroy(first);
    first = createSingleInstanceNode();
    doc = createSingleInstanceNode(true /*outOfProject*/);
    EXPECT_TRUE( bool(doc) );
    NodePtr second = createSingleInstanceNode();
    EXPECT_FALSE( bool(second) );
    destroy(second);
    destroy(doc);
    destroy(first);
    EXPECT_EQ( 0, NatronTestPlugin::counter("SingleInstance.live") );
}

// A deleted node stays alive, deactivated, in the undo history: it doesn't count.
TEST(OfxSingleInstance, DeactivatedNodeDoesNotCount)
{
    if ( !NatronTestPlugin::isAvailable(kNatronTestPluginSingleInstance) ) {
        NatronTestPlugin::reportSkipped(kNatronTestPluginSingleInstance);

        return;
    }
    NodePtr first = createSingleInstanceNode();
    ASSERT_TRUE( bool(first) );
    first->deactivate();
    NodePtr second = createSingleInstanceNode();
    EXPECT_TRUE( bool(second) );
    destroy(second);
    destroy(first);
    EXPECT_EQ( 0, NatronTestPlugin::counter("SingleInstance.live") );
}

// A project that already contains two instances (saved by an older Natron) loads both, with a
// warning in the log, instead of losing one.
TEST(OfxSingleInstance, LoadProjectWithTwoInstances)
{
    if ( !NatronTestPlugin::isAvailable(kNatronTestPluginSingleInstance) ) {
        NatronTestPlugin::reportSkipped(kNatronTestPluginSingleInstance);

        return;
    }
    QTemporaryDir tmpDir;
    ASSERT_TRUE( tmpDir.isValid() );
    AppInstancePtr app = appPTR->getTopLevelInstance();
    ProjectPtr project = app->getProject();

    // make two active instances, as an older Natron could
    NodePtr first = createSingleInstanceNode();
    ASSERT_TRUE( bool(first) );
    first->deactivate();
    NodePtr second = createSingleInstanceNode();
    ASSERT_TRUE( bool(second) );
    first->activate();
    const std::string firstName = first->getScriptName();
    const std::string secondName = second->getScriptName();
    const QString dir = tmpDir.path() + QLatin1Char('/');
    QString savedPath;
    ASSERT_TRUE( project->saveProject( dir, QString::fromUtf8("SingleInstance.ntp"), &savedPath ) );
    first.reset();
    second.reset();

    ASSERT_TRUE( project->loadProject( dir, QString::fromUtf8("SingleInstance.ntp") ) );
    EXPECT_TRUE( bool( project->getNodeByName(firstName) ) );
    EXPECT_TRUE( bool( project->getNodeByName(secondName) ) );
    EXPECT_EQ( 2, NatronTestPlugin::counter("SingleInstance.live") );

    // leave the shared app with an untitled empty project, before the directory is removed
    project->resetProject();
    EXPECT_EQ( 0, NatronTestPlugin::counter("SingleInstance.live") );
}
