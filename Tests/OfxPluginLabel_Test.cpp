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

// The label Natron shows for an OFX plug-in (AbstractOfxEffectInstance::makePluginLabel(), used for the menus and
// for the nodes): the plug-in's label, else its short label, else its long label. HostSupport gives the three labels
// the plug-in identifier as their default (openfx#384, ofxCore.h kOfxPropLabel: "Default - the kOfxPropName the
// object was created with"), so a label equal to the identifier counts as unset: a plug-in that sets only its short
// or long label shows it, not its identifier, and one that sets none shows its identifier (#2023).

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <string>

#include <gtest/gtest.h>

#include <ofxCore.h>
#include <ofxhImageEffect.h>
#include <ofxhPropertySuite.h>

#include "Engine/AppManager.h"
#include "Engine/Node.h"
#include "Engine/OfxEffectInstance.h"
#include "Engine/Plugin.h"

#include "OfxTestPlugin.h"

NATRON_NAMESPACE_USING

namespace {
// makePluginLabel() for an effect descriptor with these labels
std::string
makePluginLabel(const std::string& pluginIdentifier,
                const std::string& label,
                const std::string& shortLabel,
                const std::string& longLabel)
{
    const OFX::Host::Property::PropSpec labels[] = {
        { kOfxPropLabel, OFX::Host::Property::eString, 1, false, "" },
        { kOfxPropShortLabel, OFX::Host::Property::eString, 1, false, "" },
        { kOfxPropLongLabel, OFX::Host::Property::eString, 1, false, "" },
        OFX::Host::Property::propSpecEnd
    };
    OFX::Host::ImageEffect::Base effect(labels);

    effect.getProps().setStringProperty(kOfxPropLabel, label);
    effect.getProps().setStringProperty(kOfxPropShortLabel, shortLabel);
    effect.getProps().setStringProperty(kOfxPropLongLabel, longLabel);

    return AbstractOfxEffectInstance::makePluginLabel(pluginIdentifier, effect);
}
} // anonymous namespace

TEST(OfxPluginLabel, MakePluginLabel)
{
    const std::string id("net.natron.test.Id");

    EXPECT_EQ( "Label", makePluginLabel(id, "Label", "Short", "Long") );
    // a label that is the identifier, HostSupport's default, is unset
    EXPECT_EQ( "Short", makePluginLabel(id, id, "Short", id) );
    EXPECT_EQ( "Long", makePluginLabel(id, id, id, "Long") );
    EXPECT_EQ( id, makePluginLabel(id, id, id, id) );
    // empty labels too (HostSupport's default before openfx#384); an empty short or long label is the label
    EXPECT_EQ( "Label", makePluginLabel(id, "Label", "", "") );
    EXPECT_EQ( "Short", makePluginLabel(id, "", "Short", "") );
    EXPECT_EQ( id, makePluginLabel(id, "", "", "") );
}

namespace {
// the label of the plug-in pluginID in Natron's plug-in list (the menus) and of a node of it, which must be label
void
expectPluginLabel(const char* pluginID,
                  const std::string& label)
{
    if ( !NatronTestPlugin::isAvailable(pluginID) ) {
        NatronTestPlugin::reportMissing(pluginID);

        return;
    }
    Plugin* plugin = appPTR->getPluginBinary(QString::fromUtf8(pluginID), -1, -1, false);
    ASSERT_TRUE(plugin);
    EXPECT_EQ( label, plugin->getPluginLabel().toStdString() );

    NatronTest::NodeScope nodes;
    NodePtr node = nodes.create(pluginID);
    ASSERT_TRUE( bool(node) );
    EXPECT_EQ( label, node->getPluginLabel() );
}
} // anonymous namespace

TEST(OfxPluginLabel, ShortLabelOnly)
{
    expectPluginLabel("net.natron.test.LabelShortOnly", "ShortOnly");
}

TEST(OfxPluginLabel, LongLabelOnly)
{
    expectPluginLabel("net.natron.test.LabelLongOnly", "LongOnly");
}

TEST(OfxPluginLabel, NoLabel)
{
    expectPluginLabel("net.natron.test.LabelNone", "net.natron.test.LabelNone");
}
