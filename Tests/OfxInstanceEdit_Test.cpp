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

// kOfxActionBeginInstanceEdit / kOfxActionEndInstanceEdit (ofxCore.h): End only after Begin, once per
// editing session, and before kOfxActionDestroyInstance (#1381); kOfxPropIsInteractive is 1 between
// them and 0 otherwise (#1382). The GUI calls Node::beginEditKnobs() when the settings panel is opened
// and Node::endEditKnobs() when it is closed; the tests call them directly.

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <string>

#include <gtest/gtest.h>

#include <ofxCore.h>

#include "Engine/AppInstance.h"
#include "Engine/AppManager.h"
#include "Engine/CreateNodeArgs.h"
#include "Engine/Node.h"
#include "Engine/OfxEffectInstance.h"
#include "Engine/OfxImageEffectInstance.h"
#include "Engine/Project.h"

#include "OfxTestPlugin.h"

// Tests/OfxTestPlugin/NatronTestPluginInstanceEdit.cpp
#define kNatronTestPluginInstanceEdit "net.natron.test.InstanceEdit"

NATRON_NAMESPACE_USING

namespace {
struct InstanceEditCounters
{
    int begin, end, created, destroyed, interactiveInBegin, interactiveInEnd, errors;

    InstanceEditCounters()
    {
        begin = NatronTestPlugin::counter("InstanceEdit.begin");
        end = NatronTestPlugin::counter("InstanceEdit.end");
        created = NatronTestPlugin::counter("InstanceEdit.created");
        destroyed = NatronTestPlugin::counter("InstanceEdit.destroyed");
        interactiveInBegin = NatronTestPlugin::counter("InstanceEdit.interactiveInBegin");
        interactiveInEnd = NatronTestPlugin::counter("InstanceEdit.interactiveInEnd");
        errors = NatronTestPlugin::counter("InstanceEdit.error.endWithoutBegin") +
                 NatronTestPlugin::counter("InstanceEdit.error.beginTwice") +
                 NatronTestPlugin::counter("InstanceEdit.error.destroyWhileEditing");
    }
};

NodePtr
createInstanceEditNode()
{
    AppInstancePtr app = appPTR->getTopLevelInstance();
    CreateNodeArgs args( kNatronTestPluginInstanceEdit, app->getProject() );

    args.setProperty<bool>(kCreateNodeArgsPropAddUndoRedoCommand, false);

    return app->createNode(args);
}

// kOfxPropIsInteractive of the instance, as the host sets it
int
isInteractive(const NodePtr& node)
{
    OfxEffectInstance* effect = dynamic_cast<OfxEffectInstance*>( node->getEffectInstance().get() );

    if ( !effect || !effect->effectInstance() ) {
        return -1;
    }

    return effect->effectInstance()->getProps().getIntProperty(kOfxPropIsInteractive);
}
} // anonymous namespace

// A node that was never edited gets no kOfxActionEndInstanceEdit when it is deleted.
TEST(OfxInstanceEdit, NoEndWithoutBegin)
{
    if ( !NatronTestPlugin::isAvailable(kNatronTestPluginInstanceEdit) ) {
        NatronTestPlugin::reportSkipped(kNatronTestPluginInstanceEdit);

        return;
    }
    const InstanceEditCounters before;
    NodePtr node = createInstanceEditNode();
    ASSERT_TRUE( bool(node) );
    EXPECT_EQ( 0, isInteractive(node) );

    // deleted (undoable), then destroyed
    node->deactivate();
    node->destroyNode(true /*blockingDestroy*/, false /*autoReconnect*/);
    node.reset();

    const InstanceEditCounters after;
    EXPECT_EQ(before.created + 1, after.created);
    EXPECT_EQ(before.destroyed + 1, after.destroyed);
    EXPECT_EQ(before.begin, after.begin);
    EXPECT_EQ(before.end, after.end);
    EXPECT_EQ(before.errors, after.errors);
}

// Begin and End alternate, and kOfxPropIsInteractive is 1 only between them.
TEST(OfxInstanceEdit, BeginEndAlternate)
{
    if ( !NatronTestPlugin::isAvailable(kNatronTestPluginInstanceEdit) ) {
        NatronTestPlugin::reportSkipped(kNatronTestPluginInstanceEdit);

        return;
    }
    const InstanceEditCounters before;
    NodePtr node = createInstanceEditNode();
    ASSERT_TRUE( bool(node) );

    // the panel is opened: Begin, with kOfxPropIsInteractive = 1
    node->beginEditKnobs();
    EXPECT_EQ( 1, isInteractive(node) );
    EXPECT_TRUE( node->getEffectInstance()->isEditingKnobs() );
    // another editor (e.g. the curve editor opens the panel): no second Begin
    node->beginEditKnobs();
    // the panel is closed: End, then kOfxPropIsInteractive = 0
    node->endEditKnobs();
    EXPECT_EQ( 0, isInteractive(node) );
    EXPECT_FALSE( node->getEffectInstance()->isEditingKnobs() );
    node->endEditKnobs();
    // opened again: a new Begin
    node->beginEditKnobs();
    EXPECT_EQ( 1, isInteractive(node) );

    InstanceEditCounters mid;
    EXPECT_EQ(before.begin + 2, mid.begin);
    EXPECT_EQ(before.end + 1, mid.end);
    // the plug-in sees kOfxPropIsInteractive = 1 in Begin, and still 1 in End (the editor is closing)
    EXPECT_EQ(before.interactiveInBegin + 2, mid.interactiveInBegin);
    EXPECT_EQ(before.interactiveInEnd + 1, mid.interactiveInEnd);

    // deleting the node closes its editor: End (once), and no Begin when the deletion is undone
    // without a GUI to open the panel again
    node->deactivate();
    EXPECT_EQ( 0, isInteractive(node) );
    node->activate();
    mid = InstanceEditCounters();
    EXPECT_EQ(before.begin + 2, mid.begin);
    EXPECT_EQ(before.end + 2, mid.end);

    node->destroyNode(true /*blockingDestroy*/, false /*autoReconnect*/);
    node.reset();
    const InstanceEditCounters after;
    EXPECT_EQ(before.begin + 2, after.begin);
    EXPECT_EQ(before.end + 2, after.end);
    EXPECT_EQ(before.destroyed + 1, after.destroyed);
    EXPECT_EQ(before.errors, after.errors);
}

// An instance destroyed while it is edited (e.g. the project is closed with the panel open) gets
// kOfxActionEndInstanceEdit before kOfxActionDestroyInstance.
TEST(OfxInstanceEdit, EndBeforeDestroy)
{
    if ( !NatronTestPlugin::isAvailable(kNatronTestPluginInstanceEdit) ) {
        NatronTestPlugin::reportSkipped(kNatronTestPluginInstanceEdit);

        return;
    }
    const InstanceEditCounters before;
    NodePtr node = createInstanceEditNode();
    ASSERT_TRUE( bool(node) );
    node->beginEditKnobs();

    node->destroyNode(true /*blockingDestroy*/, false /*autoReconnect*/);
    node.reset();

    const InstanceEditCounters after;
    EXPECT_EQ(before.begin + 1, after.begin);
    EXPECT_EQ(before.end + 1, after.end);
    EXPECT_EQ(before.destroyed + 1, after.destroyed);
    EXPECT_EQ(before.errors, after.errors);
}
