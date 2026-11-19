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

#include "Engine/AppInstance.h"
#include "Engine/AppManager.h"
#include "Engine/CreateNodeArgs.h"
#include "Engine/Node.h"
#include "Engine/Project.h"

#include "OfxTestPlugin.h"

NATRON_NAMESPACE_USING

namespace {
NodePtr
createSingleInstanceNode()
{
    AppInstancePtr app = appPTR->getTopLevelInstance();
    CreateNodeArgs args( kNatronTestPluginSingleInstance, app->getProject() );

    args.setProperty<bool>(kCreateNodeArgsPropAddUndoRedoCommand, false);
    args.setProperty<bool>(kCreateNodeArgsPropSilent, true); // no error dialog

    return app->createNode(args);
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
    EXPECT_EQ( 1, NatronTestPlugin::counter("SingleInstance.maxLive") );
    if (second) {
        second->destroyNode(true /*blockingDestroy*/, false /*autoReconnect*/);
        second.reset();
    }

    // once the first one is gone, a new one may be created
    first->destroyNode(true /*blockingDestroy*/, false /*autoReconnect*/);
    first.reset();
    EXPECT_EQ( 0, NatronTestPlugin::counter("SingleInstance.live") );
    NodePtr third = createSingleInstanceNode();
    EXPECT_TRUE( bool(third) );
    EXPECT_EQ( 1, NatronTestPlugin::counter("SingleInstance.live") );
    if (third) {
        third->destroyNode(true /*blockingDestroy*/, false /*autoReconnect*/);
        third.reset();
    }
    EXPECT_EQ( 0, NatronTestPlugin::counter("SingleInstance.live") );
    EXPECT_EQ( 1, NatronTestPlugin::counter("SingleInstance.maxLive") );
}
