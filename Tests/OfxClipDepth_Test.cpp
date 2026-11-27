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

// A plug-in that doesn't support multiple clip depths (kOfxImageEffectPropSupportsMultipleClipDepths 0) may not
// set the clip depths in kOfxImageEffectActionGetClipPreferences: "the host will transparently map all of an
// effect's input and output clips to a single depth [...] the deepest depth of any input clip mapped to a depth
// the plugin supports" (ofxClipPreferences.rst). Natron makes the depth properties read-only, but accepted the
// plug-in's write (with a warning) and used it for every clip (#1385).

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <gtest/gtest.h>

#include "OfxTestPlugin.h"

#define kNatronTestPluginDepthWrite "net.natron.test.DepthWrite"

NATRON_NAMESPACE_USING

TEST(OfxClipDepth, PluginWithoutMultipleDepthsCannotSetTheDepth)
{
    if ( !NatronTestPlugin::isAvailable(kNatronTestPluginDepthWrite) || !NatronTestPlugin::isAvailable(kNatronTestPluginGenerator) ) {
        NatronTestPlugin::reportMissing(kNatronTestPluginDepthWrite);

        return;
    }
    // a float input; the plug-in supports 8-bit and float images, and sets its output depth to 8 bits
    NodePtr generator = NatronTestPlugin::createNode(kNatronTestPluginGenerator);
    NodePtr node = NatronTestPlugin::createNode(kNatronTestPluginDepthWrite);
    ASSERT_TRUE( generator && node );
    ASSERT_EQ( eImageBitDepthFloat, generator->getEffectInstance()->getBitDepth(-1) );
    ASSERT_TRUE( node->getApp()->getProject()->connectNodes(0, generator, node) );

    // the clip preferences: every clip is float, the deepest input depth that the plug-in supports
    EXPECT_EQ( eImageBitDepthFloat, node->getEffectInstance()->getBitDepth(-1) );
    EXPECT_EQ( eImageBitDepthFloat, node->getEffectInstance()->getBitDepth(0) );

    // what the plug-in sees in render: its render fails unless Source and Output are float images
    EXPECT_TRUE( bool( NatronTestPlugin::renderFrame(node, 1.) ) );

    node->destroyNode(true, false);
    generator->destroyNode(true, false);
}
