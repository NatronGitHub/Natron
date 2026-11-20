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


// Plug-ins that set kOfxImageEffectPluginPropObsolete (OFX, after 1.5.1) load in old projects, but
// Natron doesn't offer them to users for new nodes: they are hidden from the menus, the node-creation
// dialog and the documentation, and they are not registered as readers or writers. They can still be
// created by ID with kCreateNodeArgsPropAllowNonUserCreatablePlugins, which is what project loading and
// the Python API (app.createNode()) do. The Natron extension kNatronOfxImageEffectPropDeprecated, which
// had the same meaning, is not read any more (#1491).

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <gtest/gtest.h>

#include <ofxImageEffect.h>
#include <ofxhPropertySuite.h>

#include "Engine/AppInstance.h"
#include "Engine/AppManager.h"
#include "Engine/CreateNodeArgs.h"
#include "Engine/DocumentationGroups.h"
#include "Engine/Node.h"
#include "Engine/OfxHost.h"
#include "Engine/Plugin.h"
#include "Engine/Project.h"

#include "OfxTestPlugin.h"

NATRON_NAMESPACE_USING

#define kNatronTestPluginObsolete "net.natron.test.Obsolete"
#define kNatronTestPluginNatronDeprecated "net.natron.test.NatronDeprecated"

namespace {
// kOfxImageEffectPluginPropObsolete, as HostSupport declares it on every image effect descriptor, and
// the former Natron extension property, whose #define is removed from ofxNatron.h (NatronGitHub/openfx#198).
const char* const kFormerNatronDeprecatedProp = "NatronOfxImageEffectPropDeprecated";
const OFX::Host::Property::PropSpec descriptorProps[] = {
    { kOfxImageEffectPluginPropObsolete, OFX::Host::Property::eInt, 1, false, "0" },
    { kFormerNatronDeprecatedProp, OFX::Host::Property::eInt, 1, false, "0" },
    OFX::Host::Property::propSpecEnd
};

NodePtr
createNodeByID(const char* pluginID,
               bool allowNonUserCreatable)
{
    AppInstancePtr app = appPTR->getTopLevelInstance();
    CreateNodeArgs args( pluginID, app->getProject() );

    args.setProperty<bool>(kCreateNodeArgsPropAddUndoRedoCommand, false);
    args.setProperty<bool>(kCreateNodeArgsPropAutoConnect, false);
    args.setProperty<bool>(kCreateNodeArgsPropSettingsOpened, false);
    args.setProperty<bool>(kCreateNodeArgsPropAllowNonUserCreatablePlugins, allowNonUserCreatable);

    return app->createNode(args);
}
}

TEST(OfxPluginObsolete, Default)
{
    OFX::Host::Property::Set props(descriptorProps);

    EXPECT_FALSE( NATRON_NAMESPACE::OfxHost::isPluginObsolete(props) );
}

TEST(OfxPluginObsolete, Obsolete)
{
    OFX::Host::Property::Set props(descriptorProps);

    props.setIntProperty(kOfxImageEffectPluginPropObsolete, 1);
    EXPECT_TRUE( NATRON_NAMESPACE::OfxHost::isPluginObsolete(props) );
}

// the Natron extension property is ignored (#1491)
TEST(OfxPluginObsolete, FormerNatronDeprecatedIgnored)
{
    OFX::Host::Property::Set props(descriptorProps);

    props.setIntProperty(kFormerNatronDeprecatedProp, 1);
    EXPECT_FALSE( NATRON_NAMESPACE::OfxHost::isPluginObsolete(props) );
}

// a plug-in that sets kOfxImageEffectPluginPropObsolete is loaded but not offered for new nodes, and
// can be created by ID
TEST(OfxPluginObsolete, HiddenButCreatableByID)
{
    if ( !NatronTestPlugin::isAvailable(kNatronTestPluginObsolete) ) {
        NatronTestPlugin::reportSkipped(kNatronTestPluginObsolete);

        return;
    }
    Plugin* plugin = appPTR->getPluginBinary(QString::fromUtf8(kNatronTestPluginObsolete), -1, -1, false);
    ASSERT_TRUE(plugin);
    EXPECT_TRUE( plugin->getIsDeprecated() );
    // getIsUserCreatable() is what the node menus, the toolbars, the Tab node-creation dialog and the
    // plug-in shortcuts check (AppManager::onAllPluginsLoaded(), GuiApplicationManager::onPluginLoaded(),
    // NodeCreationDialog), and what AppInstance::createNode() checks without
    // kCreateNodeArgsPropAllowNonUserCreatablePlugins
    EXPECT_FALSE( plugin->getIsUserCreatable() );
    EXPECT_FALSE( DocumentationGroups::isDocumented(plugin) );

    // the user can't create it...
    EXPECT_FALSE( bool( createNodeByID(kNatronTestPluginObsolete, false) ) );

    // ...but a project or a Python script can (both set kCreateNodeArgsPropAllowNonUserCreatablePlugins)
    NodePtr node = createNodeByID(kNatronTestPluginObsolete, true);
    ASSERT_TRUE( bool(node) );
    EXPECT_EQ( std::string(kNatronTestPluginObsolete), node->getPluginID() );
    node->destroyNode(true /*blockingDestroy*/, false /*autoReconnect*/);
}

// a plug-in that sets only the former Natron extension property is offered for new nodes (#1491)
TEST(OfxPluginObsolete, FormerNatronDeprecatedPluginIsUserCreatable)
{
    if ( !NatronTestPlugin::isAvailable(kNatronTestPluginNatronDeprecated) ) {
        NatronTestPlugin::reportSkipped(kNatronTestPluginNatronDeprecated);

        return;
    }
    Plugin* plugin = appPTR->getPluginBinary(QString::fromUtf8(kNatronTestPluginNatronDeprecated), -1, -1, false);
    ASSERT_TRUE(plugin);
    EXPECT_FALSE( plugin->getIsDeprecated() );
    EXPECT_TRUE( plugin->getIsUserCreatable() );

    NodePtr node = createNodeByID(kNatronTestPluginNatronDeprecated, false);
    ASSERT_TRUE( bool(node) );
    node->destroyNode(true /*blockingDestroy*/, false /*autoReconnect*/);
}
