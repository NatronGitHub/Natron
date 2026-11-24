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
            NatronTestPlugin::reportSkipped(ids[i]);

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

// connect source to the Mask input of a new Masked node, and return its mask channel menu
KnobChoicePtr
maskChannelWithInput(const NodePtr& source)
{
    NodePtr masked = NatronTestPlugin::createNode(kNatronTestPluginMasked);

    if (!masked) {
        return KnobChoicePtr();
    }
    const int mask = inputNamed(masked, "Mask");
    if ( (mask < 0) || !masked->connectInput(source, mask) ) {
        return KnobChoicePtr();
    }

    return std::dynamic_pointer_cast<KnobChoice>( masked->getKnobByName("maskChannel_Mask") );
}
} // anonymous namespace

TEST(MaskChannel, DefaultIsAlpha)
{
    if ( !pluginsAvailable() ) {
        return;
    }
    const std::string alphaID = ImagePlaneDesc::getRGBAComponents().getChannelOption(3).id;

    // alpha-only input: None, Color.A
    KnobChoicePtr alphaOnly = maskChannelWithInput( NatronTestPlugin::createNode(kNatronTestPluginAlphaGenerator) );
    ASSERT_TRUE( bool(alphaOnly) );
    ASSERT_EQ( 2, alphaOnly->getNumEntries() );
    EXPECT_EQ( 1, alphaOnly->getValue() );
    EXPECT_EQ( alphaID, alphaOnly->getActiveEntry().id );

    // RGBA input: None, Color.R, Color.G, Color.B, Color.A
    KnobChoicePtr rgba = maskChannelWithInput( NatronTestPlugin::createNode(kNatronTestPluginGenerator) );
    ASSERT_TRUE( bool(rgba) );
    ASSERT_EQ( 5, rgba->getNumEntries() );
    EXPECT_EQ( 4, rgba->getValue() );
    EXPECT_EQ( alphaID, rgba->getActiveEntry().id );

    // leave the shared app with an untitled empty project
    appPTR->getTopLevelInstance()->getProject()->resetProject();
}
