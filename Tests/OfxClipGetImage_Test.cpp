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

// clipGetImage, called again in the same render for a larger region, must return an image that covers
// that region (clipped to the RoD), and two calls must return two handles (#1377).

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <gtest/gtest.h>

#include "OfxTestPlugin.h"

NATRON_NAMESPACE_USING

TEST(OfxClipGetImage, LargerRegionAndSeparateHandles)
{
    if ( !NatronTestPlugin::isAvailable(kNatronTestPluginClipGetImage) || !NatronTestPlugin::isAvailable(kNatronTestPluginGenerator) ) {
        NatronTestPlugin::reportSkipped(kNatronTestPluginClipGetImage);

        return;
    }
    NodePtr source = NatronTestPlugin::createNode(kNatronTestPluginGenerator);
    NodePtr node = NatronTestPlugin::createNode(kNatronTestPluginClipGetImage);
    ASSERT_TRUE( source && node );
    ASSERT_TRUE( node->getApp()->getProject()->connectNodes(0, source, node) );

    const int checks = NatronTestPlugin::counter("ClipGetImage.checks");
    const int fetchFailed = NatronTestPlugin::counter("ClipGetImage.fetchFailed");
    const int regionErrors = NatronTestPlugin::counter("ClipGetImage.regionErrors");
    const int sameHandle = NatronTestPlugin::counter("ClipGetImage.sameHandle");
    ASSERT_TRUE( bool( NatronTestPlugin::renderFrame(node, 1.) ) );
    ASSERT_EQ( checks + 1, NatronTestPlugin::counter("ClipGetImage.checks") );
    EXPECT_EQ( fetchFailed, NatronTestPlugin::counter("ClipGetImage.fetchFailed") );
    EXPECT_EQ( regionErrors, NatronTestPlugin::counter("ClipGetImage.regionErrors") ) << "an image does not cover the requested region";
    EXPECT_EQ( sameHandle, NatronTestPlugin::counter("ClipGetImage.sameHandle") ) << "two calls returned the same image handle";

    node->destroyNode(true, false);
    source->destroyNode(true, false);
}
