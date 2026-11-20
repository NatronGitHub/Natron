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

// The inArgs of kOfxImageEffectActionBeginSequenceRender and kOfxImageEffectActionEndSequenceRender
// (ofxImageEffect.h, #1385): kOfxImageEffectPropFrameStep "can be any positive value" (the End of a single
// frame render passed the frame as its step, so 0 at frame 0 and negative before), and End gets the frame
// range, frame step, kOfxPropIsInteractive and kOfxImageEffectPropInteractiveRenderStatus of its Begin.
// The test plug-in net.natron.test.SequenceArgs (Tests/OfxTestPlugin/NatronTestPluginSequenceArgs.cpp)
// counts the arguments that are wrong.

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <gtest/gtest.h>

#include "Engine/AppManager.h"
#include "Engine/Node.h"

#include "OfxTestPlugin.h"

#define kSequenceArgsPluginID "net.natron.test.SequenceArgs"

NATRON_NAMESPACE_USING

namespace {
void
resetCounters()
{
    const char* names[] = { "SequenceArgs.begin", "SequenceArgs.end", "SequenceArgs.badStep", "SequenceArgs.mismatch", "SequenceArgs.interactive" };

    for (std::size_t i = 0; i < sizeof(names) / sizeof(names[0]); ++i) {
        NatronTestPlugin::setCounter(names[i], 0);
    }
}
} // anonymous namespace

// single frame renders (one Begin/End pair per render), at frames 0, -2 and 3
TEST(OfxSequenceRenderArgs, SingleFrameRender)
{
    if ( !NatronTestPlugin::isAvailable(kSequenceArgsPluginID) ) {
        NatronTestPlugin::reportSkipped(kSequenceArgsPluginID);

        return;
    }
    NodePtr node = NatronTestPlugin::createNode(kSequenceArgsPluginID);
    ASSERT_TRUE(node != 0);
    resetCounters();
    const double times[] = { 0., -2., 3. };
    for (std::size_t i = 0; i < sizeof(times) / sizeof(times[0]); ++i) {
        // render at different times, so that the cache does not skip the render
        EXPECT_TRUE( NatronTestPlugin::renderFrame(node, times[i]) != 0 ) << times[i];
    }
    EXPECT_GE(NatronTestPlugin::counter("SequenceArgs.begin"), 3);
    EXPECT_EQ( NatronTestPlugin::counter("SequenceArgs.begin"), NatronTestPlugin::counter("SequenceArgs.end") );
    EXPECT_EQ( 0, NatronTestPlugin::counter("SequenceArgs.badStep") );
    EXPECT_EQ( 0, NatronTestPlugin::counter("SequenceArgs.mismatch") );
    // the tests run in background mode: no render is "due to user interaction in a GUI"
    EXPECT_EQ( 0, NatronTestPlugin::counter("SequenceArgs.interactive") );
    node->destroyNode(true /*blockingDestroy*/, false /*autoReconnect*/);
}
