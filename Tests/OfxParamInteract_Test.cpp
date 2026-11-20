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

// "An interact instance must always be associated with an effect instance, so it gets created after
// the effect and destroyed before the effect" (ofxInteracts.rst). Natron destroyed the parameter
// interacts after their effect instance, so their kOfxActionDestroyInstance saw a dangling
// kOfxPropEffectInstance (#1362).

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

TEST(OfxParamInteract, DestroyedBeforeEffect)
{
    if ( !NatronTestPlugin::isAvailable(kNatronTestPluginParamInteract) ) {
        NatronTestPlugin::reportSkipped(kNatronTestPluginParamInteract);

        return;
    }
    const int createdBefore = NatronTestPlugin::counter("ParamInteract.interactCreatedBeforeEffect");
    const int createdAfter = NatronTestPlugin::counter("ParamInteract.interactCreatedAfterEffect");
    const int destroyedBefore = NatronTestPlugin::counter("ParamInteract.interactDestroyedBeforeEffect");
    const int destroyedAfter = NatronTestPlugin::counter("ParamInteract.interactDestroyedAfterEffect");

    AppInstancePtr app = appPTR->getTopLevelInstance();
    CreateNodeArgs args( kNatronTestPluginParamInteract, app->getProject() );
    args.setProperty<bool>(kCreateNodeArgsPropAddUndoRedoCommand, false);
    NodePtr node = app->createNode(args);
    ASSERT_TRUE( bool(node) );
    EXPECT_EQ( createdBefore, NatronTestPlugin::counter("ParamInteract.interactCreatedBeforeEffect") );
    EXPECT_EQ( createdAfter + 1, NatronTestPlugin::counter("ParamInteract.interactCreatedAfterEffect") );

    node->destroyNode(true /*blockingDestroy*/, false /*autoReconnect*/);
    node.reset();
    EXPECT_EQ( destroyedBefore + 1, NatronTestPlugin::counter("ParamInteract.interactDestroyedBeforeEffect") );
    EXPECT_EQ( destroyedAfter, NatronTestPlugin::counter("ParamInteract.interactDestroyedAfterEffect") );
}
