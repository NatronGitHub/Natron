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

// The OFX test plug-ins built with the tests (Tests/OfxTestPlugin.h) load, and the host creates and
// destroys their instances.

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

TEST(OfxTestPlugin, CreateAndDestroyInstance)
{
    const char* pluginIDs[] = {
        kNatronTestPluginParamInteract, kNatronTestPluginSingleInstance, kNatronTestPluginSyncPrivateData
    };
    const char* counterPrefixes[] = {
        "ParamInteract", "SingleInstance", "SyncPrivateData"
    };

    for (int i = 0; i < 3; ++i) {
        if ( !NatronTestPlugin::isAvailable(pluginIDs[i]) ) {
            NatronTestPlugin::reportSkipped(pluginIDs[i]);

            return;
        }
        const std::string prefix(counterPrefixes[i]);
        const int created = NatronTestPlugin::counter(prefix + ".created");
        const int destroyed = NatronTestPlugin::counter(prefix + ".destroyed");
        ASSERT_GE(created, 0);

        AppInstancePtr app = appPTR->getTopLevelInstance();
        CreateNodeArgs args( pluginIDs[i], app->getProject() );
        args.setProperty<bool>(kCreateNodeArgsPropAddUndoRedoCommand, false);
        NodePtr node = app->createNode(args);
        ASSERT_TRUE( bool(node) ) << pluginIDs[i];
        EXPECT_EQ( created + 1, NatronTestPlugin::counter(prefix + ".created") ) << pluginIDs[i];

        node->destroyNode(true /*blockingDestroy*/, false /*autoReconnect*/);
        node.reset();
        EXPECT_EQ( destroyed + 1, NatronTestPlugin::counter(prefix + ".destroyed") ) << pluginIDs[i];
    }
}
