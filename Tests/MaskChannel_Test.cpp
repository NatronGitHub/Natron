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

// The mask channel of a mask input is the alpha channel by default, also when the input is alpha-only (#1685):
// its menu is then None, Color.A, and the default (the 5th entry, Color.A of an RGBA input) selected nothing, so
// that the mask had no effect (e.g. in the EdgeBlur PyPlug). The test plug-ins are in
// Tests/OfxTestPlugin/NatronTestPluginMaskChannel.cpp.

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <string>

#include <gtest/gtest.h>

#include "Engine/AppInstance.h"
#include "Engine/AppManager.h"
#include "Engine/ImagePlaneDesc.h"
#include "Engine/KnobTypes.h"
#include "Engine/Node.h"
#include "Engine/Project.h"

#include "OfxTestPlugin.h"

#define kNatronTestPluginAlphaGenerator "net.natron.test.AlphaGenerator"
#define kNatronTestPluginMasked "net.natron.test.Masked"

NATRON_NAMESPACE_USING

namespace {
bool
pluginsAvailable()
{
    const char* const ids[] = { kNatronTestPluginAlphaGenerator, kNatronTestPluginMasked, kNatronTestPluginGenerator };

    for (int i = 0; i < 3; ++i) {
        if ( !NatronTestPlugin::isAvailable(ids[i]) ) {
            NatronTestPlugin::reportMissing(ids[i]);

            return false;
        }
    }

    return true;
}

int
inputNamed(const NodePtr& node,
           const std::string& label)
{
    for (int i = 0; i < node->getNInputs(); ++i) {
        if (node->getInputLabel(i) == label) {
            return i;
        }
    }

    return -1;
}

KnobChoicePtr
maskChannel(const NodePtr& masked)
{
    return std::dynamic_pointer_cast<KnobChoice>( masked->getKnobByName("maskChannel_Mask") );
}

// connect source to the Mask input of masked
bool
connectMask(const NodePtr& masked,
            const NodePtr& source)
{
    const int mask = inputNamed(masked, "Mask");

    return (mask >= 0) && masked->connectInput(source, mask);
}
} // anonymous namespace

TEST(MaskChannel, DefaultIsAlpha)
{
    if ( !pluginsAvailable() ) {
        return;
    }
    const NatronTest::ProjectResetScope resetProject;
    const std::string alphaID = ImagePlaneDesc::getRGBAComponents().getChannelOption(3).id;
    NodePtr alphaGenerator = NatronTestPlugin::createNode(kNatronTestPluginAlphaGenerator);
    NodePtr rgbaGenerator = NatronTestPlugin::createNode(kNatronTestPluginGenerator);
    NodePtr alphaMasked = NatronTestPlugin::createNode(kNatronTestPluginMasked);
    NodePtr rgbaMasked = NatronTestPlugin::createNode(kNatronTestPluginMasked);
    NodePtr chosenMasked = NatronTestPlugin::createNode(kNatronTestPluginMasked);
    ASSERT_TRUE(alphaGenerator && rgbaGenerator && alphaMasked && rgbaMasked && chosenMasked);
    KnobChoicePtr alphaOnly = maskChannel(alphaMasked);
    KnobChoicePtr rgba = maskChannel(rgbaMasked);
    KnobChoicePtr chosen = maskChannel(chosenMasked);
    ASSERT_TRUE(alphaOnly && rgba && chosen);

    // not connected: the menu has no Color.A, and none is selected (no "no longer exists" warning)
    EXPECT_TRUE( alphaOnly->isActiveEntryPresentInEntries() );

    // alpha-only input: None, Color.A
    ASSERT_TRUE( connectMask(alphaMasked, alphaGenerator) );
    ASSERT_EQ( 2, alphaOnly->getNumEntries() );
    EXPECT_EQ( 1, alphaOnly->getValue() );
    EXPECT_EQ( alphaID, alphaOnly->getActiveEntry().id );

    // RGBA input: None, Color.R, Color.G, Color.B, Color.A
    ASSERT_TRUE( connectMask(rgbaMasked, rgbaGenerator) );
    ASSERT_EQ( 5, rgba->getNumEntries() );
    EXPECT_EQ( 4, rgba->getValue() );
    EXPECT_EQ( alphaID, rgba->getActiveEntry().id );

    // a channel chosen before the input is connected (e.g. by a script), out of the range of the
    // menu then, is kept
    chosen->setValue(2); // Color.G of an RGBA input
    ASSERT_TRUE( connectMask(chosenMasked, rgbaGenerator) );
    EXPECT_EQ( 2, chosen->getValue() );
    EXPECT_EQ( ImagePlaneDesc::getRGBAComponents().getChannelOption(1).id, chosen->getActiveEntry().id );
}
