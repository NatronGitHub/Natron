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

// kOfxParamPropHasHostOverlayHandle (#1932): "If set to 1, then the host is flagging that there is some sort
// of native user overlay interface handle available for the given parameter" (OFX 1.5.1 ofxParam.h). Natron
// has a native point handle for the Double2D parameters of type kOfxParamDoubleTypeXYAbsolute and
// kOfxParamDoubleTypeNormalisedXYAbsolute. The plug-in sets the type after defining the parameter: Natron
// used to compute the property before that, so it was always 0, and a plug-in that asks for the native
// handle only where the host has one (kOfxParamPropUseHostOverlayHandle) never got it.

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <string>

#include <gtest/gtest.h>

#include "Engine/KnobTypes.h"
#include "Engine/Node.h"

#include "OfxTestPlugin.h"

// Tests/OfxTestPlugin/NatronTestPluginHostOverlayHandle.cpp
#define kNatronTestPluginHostOverlayHandle "net.natron.test.HostOverlayHandle"

NATRON_NAMESPACE_USING

namespace {
// whether Natron draws its native handle for the Double2D parameter name of node
bool
hasNativeHandle(const NodePtr& node,
                const std::string& name)
{
    KnobDoublePtr knob = std::dynamic_pointer_cast<KnobDouble>( node->getKnobByName(name) );

    return knob && knob->getHasHostOverlayHandle();
}
}

TEST(OfxHostOverlayHandle, SetForXYAbsoluteTypes)
{
    if ( !NatronTestPlugin::isAvailable(kNatronTestPluginHostOverlayHandle) ) {
        NatronTestPlugin::reportMissing(kNatronTestPluginHostOverlayHandle);

        return;
    }
    NodePtr node = NatronTestPlugin::createNode(kNatronTestPluginHostOverlayHandle);
    ASSERT_TRUE( bool(node) );

    // what the plug-in read in kOfxImageEffectActionDescribeInContext, after setting the type
    EXPECT_EQ( 1, NatronTestPlugin::counter("HostOverlayHandle.xyAbsolute.has") );
    EXPECT_EQ( 1, NatronTestPlugin::counter("HostOverlayHandle.normalisedXYAbsolute.has") );
    EXPECT_EQ( 0, NatronTestPlugin::counter("HostOverlayHandle.xy.has") );
    EXPECT_EQ( 0, NatronTestPlugin::counter("HostOverlayHandle.plain.has") );
    // the value follows the type when the plug-in changes it
    EXPECT_EQ( 0, NatronTestPlugin::counter("HostOverlayHandle.changedToXY.has") );

    // the parameter instances have the descriptors' values (overlays such as openfx-supportext's
    // PositionInteract read them there)
    EXPECT_EQ( 1, NatronTestPlugin::counter("HostOverlayHandle.xyAbsolute.instanceHas") );
    EXPECT_EQ( 1, NatronTestPlugin::counter("HostOverlayHandle.normalisedXYAbsolute.instanceHas") );
    EXPECT_EQ( 0, NatronTestPlugin::counter("HostOverlayHandle.xy.instanceHas") );
    EXPECT_EQ( 0, NatronTestPlugin::counter("HostOverlayHandle.plain.instanceHas") );
    EXPECT_EQ( 0, NatronTestPlugin::counter("HostOverlayHandle.changedToXY.instanceHas") );

    // the plug-in has an overlay interact: Natron draws its native handles where the plug-in asked for them
    // (kOfxParamPropUseHostOverlayHandle, set to the value read)
    EXPECT_TRUE( hasNativeHandle(node, "xyAbsolute") );
    EXPECT_TRUE( hasNativeHandle(node, "normalisedXYAbsolute") );
    EXPECT_FALSE( hasNativeHandle(node, "xy") );
    EXPECT_FALSE( hasNativeHandle(node, "plain") );
    EXPECT_FALSE( hasNativeHandle(node, "changedToXY") );

    node->destroyNode(true /*blockingDestroy*/, false /*autoReconnect*/);
}
