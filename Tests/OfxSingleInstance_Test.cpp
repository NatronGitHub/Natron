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
#include "Engine/LogEntry.h"
#include "Engine/Node.h"
#include "Engine/NodeGroup.h"
#include "Engine/OfxEffectInstance.h"
#include "Engine/Project.h"

#include "OfxTestPlugin.h"

NATRON_NAMESPACE_USING

namespace {
NodePtr
createSingleInstanceNode(bool outOfProject = false,
                         const NodeCollectionPtr& group = NodeCollectionPtr())
{
    AppInstancePtr app = appPTR->getTopLevelInstance();
    CreateNodeArgs args( kNatronTestPluginSingleInstance, group ? group : NodeCollectionPtr( app->getProject() ) );

    args.setProperty<bool>(kCreateNodeArgsPropAddUndoRedoCommand, false);
    args.setProperty<bool>(kCreateNodeArgsPropSilent, true); // no error dialog
    if (outOfProject) {
        args.setProperty<bool>(kCreateNodeArgsPropOutOfProject, true);
        args.setProperty<bool>(kCreateNodeArgsPropNoNodeGUI, true);
    }

    return app->createNode(args);
}

// the number of "allows only one instance" warnings in the log
int
countSingleInstanceWarnings()
{
    std::list<LogEntry> entries;
    appPTR->getErrorLog_mt_safe(&entries);
    int n = 0;
    for (std::list<LogEntry>::const_iterator it = entries.begin(); it != entries.end(); ++it) {
        if ( it->message.contains( QString::fromUtf8("allows only one instance at a time") ) ) {
            ++n;
        }
    }

    return n;
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
    // and it doesn't keep its plug-in instance (#1402)
    EXPECT_EQ( 0, NatronTestPlugin::counter("SingleInstance.live") );
    NodePtr second = createSingleInstanceNode();
    EXPECT_TRUE( bool(second) );
    EXPECT_EQ( 1, NatronTestPlugin::counter("SingleInstance.live") );
    destroy(second);
    // destroying a deactivated node doesn't destroy its plug-in instance twice
    const int destroyed = NatronTestPlugin::counter("SingleInstance.destroyed");
    destroy(first);
    EXPECT_EQ( destroyed, NatronTestPlugin::counter("SingleInstance.destroyed") );
    EXPECT_EQ( 0, NatronTestPlugin::counter("SingleInstance.live") );
}

// #1402: delete a node (deactivate it, as the GUI does, keeping it for undo), create a new one,
// then undo both and redo them: there is never more than one plug-in instance, and the deleted node
// gets a new plug-in instance when it is activated again.
TEST(OfxSingleInstance, UndoDeleteRecreatesTheInstance)
{
    if ( !NatronTestPlugin::isAvailable(kNatronTestPluginSingleInstance) ) {
        NatronTestPlugin::reportSkipped(kNatronTestPluginSingleInstance);

        return;
    }
    NatronTestPlugin::setCounter("SingleInstance.maxLive", 0);
    const int created = NatronTestPlugin::counter("SingleInstance.created");
    const int destroyed = NatronTestPlugin::counter("SingleInstance.destroyed");
    NodePtr a = createSingleInstanceNode();
    ASSERT_TRUE( bool(a) );
    OfxEffectInstance* aEffect = dynamic_cast<OfxEffectInstance*>( a->getEffectInstance().get() );
    ASSERT_TRUE(aEffect != NULL);

    // delete A
    a->deactivate();
    EXPECT_TRUE( aEffect->isSingleInstanceSuspended() );
    EXPECT_EQ( destroyed + 1, NatronTestPlugin::counter("SingleInstance.destroyed") );
    EXPECT_EQ( 0, NatronTestPlugin::counter("SingleInstance.live") );
    // the plug-in gets no action while its instance is destroyed
    EXPECT_FALSE( aEffect->isEffectCreated() );

    // create B
    NodePtr b = createSingleInstanceNode();
    ASSERT_TRUE( bool(b) );
    EXPECT_EQ( 1, NatronTestPlugin::counter("SingleInstance.live") );

    // undo the creation of B, then the deletion of A
    b->deactivate();
    EXPECT_EQ( 0, NatronTestPlugin::counter("SingleInstance.live") );
    a->activate();
    EXPECT_FALSE( aEffect->isSingleInstanceSuspended() );
    EXPECT_TRUE( aEffect->isEffectCreated() );
    EXPECT_EQ( 1, NatronTestPlugin::counter("SingleInstance.live") );
    EXPECT_EQ( created + 3, NatronTestPlugin::counter("SingleInstance.created") ); // A, B, and A again
    // while A is active, a new node C is still refused
    NodePtr c = createSingleInstanceNode();
    EXPECT_FALSE( bool(c) );
    destroy(c);

    // redo both
    a->deactivate();
    b->activate();
    EXPECT_EQ( 1, NatronTestPlugin::counter("SingleInstance.live") );
    EXPECT_EQ( 1, NatronTestPlugin::counter("SingleInstance.maxLive") );

    destroy(a);
    destroy(b);
    EXPECT_EQ( 0, NatronTestPlugin::counter("SingleInstance.live") );
    // every instance the plug-in created was destroyed once
    EXPECT_EQ( NatronTestPlugin::counter("SingleInstance.created") - created,
               NatronTestPlugin::counter("SingleInstance.destroyed") - destroyed );
}

// Undoing a deletion can't be refused: if another node of the plug-in is active (e.g. created in
// another node graph, which has its own undo history), the node is activated with a warning.
TEST(OfxSingleInstance, ActivateWithAnotherActiveInstance)
{
    if ( !NatronTestPlugin::isAvailable(kNatronTestPluginSingleInstance) ) {
        NatronTestPlugin::reportSkipped(kNatronTestPluginSingleInstance);

        return;
    }
    NodePtr a = createSingleInstanceNode();
    ASSERT_TRUE( bool(a) );
    a->deactivate();
    NodePtr b = createSingleInstanceNode();
    ASSERT_TRUE( bool(b) );
    const int warnings = countSingleInstanceWarnings();
    a->activate();
    EXPECT_TRUE( a->isActivated() );
    EXPECT_EQ( warnings + 1, countSingleInstanceWarnings() );
    EXPECT_EQ( 2, NatronTestPlugin::counter("SingleInstance.live") );
    // with two active nodes, a third one is refused
    NodePtr c = createSingleInstanceNode();
    EXPECT_FALSE( bool(c) );
    destroy(c);
    destroy(a);
    destroy(b);
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

// The undo of Group From Selection deactivates the group (and the copies in it) before it activates
// the originals, and the undo of Expand Group deactivates the expanded nodes before it activates the
// group: a node in a deactivated group holds no plug-in instance, and no warning is logged.
TEST(OfxSingleInstance, GroupDeactivationSuspendsItsNodes)
{
    if ( !NatronTestPlugin::isAvailable(kNatronTestPluginSingleInstance) ) {
        NatronTestPlugin::reportSkipped(kNatronTestPluginSingleInstance);

        return;
    }
    AppInstancePtr app = appPTR->getTopLevelInstance();
    CreateNodeArgs groupArgs( PLUGINID_NATRON_GROUP, app->getProject() );
    groupArgs.setProperty<bool>(kCreateNodeArgsPropAddUndoRedoCommand, false);
    groupArgs.setProperty<bool>(kCreateNodeArgsPropSilent, true);
    NodePtr group = app->createNode(groupArgs);
    ASSERT_TRUE( bool(group) );
    NodeGroupPtr isGroup = std::dynamic_pointer_cast<NodeGroup>( group->getEffectInstance() );
    ASSERT_TRUE( bool(isGroup) );

    // Group From Selection: the original is deleted, a copy is in the group
    NatronTestPlugin::setCounter("SingleInstance.maxLive", 0);
    NodePtr original = createSingleInstanceNode();
    ASSERT_TRUE( bool(original) );
    original->deactivate();
    NodePtr copy = createSingleInstanceNode(false, isGroup);
    ASSERT_TRUE( bool(copy) );
    EXPECT_EQ( 1, NatronTestPlugin::counter("SingleInstance.live") );
    const int warnings = countSingleInstanceWarnings();

    // its undo, in GroupFromSelectionCommand::undo()'s order
    group->deactivate();
    EXPECT_FALSE( copy->isActivated() );
    EXPECT_EQ( 0, NatronTestPlugin::counter("SingleInstance.live") );
    original->activate();
    EXPECT_EQ( 1, NatronTestPlugin::counter("SingleInstance.live") );
    // and the redo
    original->deactivate();
    group->activate();
    EXPECT_TRUE( copy->isActivated() );
    EXPECT_EQ( 1, NatronTestPlugin::counter("SingleInstance.live") );
    EXPECT_EQ( 1, NatronTestPlugin::counter("SingleInstance.maxLive") );
    EXPECT_EQ( warnings, countSingleInstanceWarnings() );

    destroy(original);
    destroy(copy);
    destroy(group);
    EXPECT_EQ( 0, NatronTestPlugin::counter("SingleInstance.live") );
}
