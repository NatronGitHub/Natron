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

// A plug-in that puts its frames in order itself, while Natron renders the frames of a render on disk in parallel
// (WriteFFmpeg, kOfxImageEffectInstancePropSequentialRender = 1), waits in the render action of frame t until frame
// t-1 is done, with the source images it fetched. The render of frame t-1 may have to render into one of those
// images: a cached image of a node upstream that both frames use (in the soak test of #248, the image of a Reformat
// that a Transform passes through at frame t). It waited for the write lock of that image, which the source image of
// frame t held for reading: the render stalled. net.natron.test.InOrder (Tests/OfxTestPlugin/NatronTestPluginInOrder.cpp)
// builds that case: frame t fetches part of Source at t-1, and frame t-1 then needs all of it (Source is frame-varying
// and cached), so Natron grows that cached image before the render action of frame t-1. InOrder waits at most 5 s for
// the previous frame, and counts the waits that timed out.

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <utility>
#include <vector>

#include <gtest/gtest.h>

#include "Engine/Knob.h"
#include "Engine/KnobTypes.h"
#include "Engine/Node.h"
#include "Engine/Project.h"

#include "OfxTestPlugin.h"

NATRON_NAMESPACE_USING

namespace {
const int kFirstFrame = 1;
const int kLastFrame = 12;

// Destroys nodes at the end of a scope (R.1)
class NodesGuard
{
public:
    explicit NodesGuard(std::vector<NodePtr> nodes)
        : _nodes( std::move(nodes) )
    {
    }

    ~NodesGuard()
    {
        for (const NodePtr& node : _nodes) {
            if (node) {
                node->destroyNode(true, false);
            }
        }
    }

    NodesGuard(const NodesGuard&) = delete;
    NodesGuard& operator=(const NodesGuard&) = delete;

private:
    std::vector<NodePtr> _nodes;
};
}

TEST(InOrderRender, PreviousFrameCanRenderIntoAHeldSourceImage)
{
    for (const char* id : { kNatronTestPluginInOrder, kNatronTestPluginSequentialStatus, kNatronTestPluginGenerator }) {
        if ( !NatronTestPlugin::isAvailable(id) ) {
            NatronTestPlugin::reportMissing(id);

            return;
        }
    }
    // Source: a frame-varying node (SequentialStatus, rendered in parallel), so that each frame has its own image
    NodePtr generator = NatronTestPlugin::createNode(kNatronTestPluginGenerator);
    NodePtr source = NatronTestPlugin::createNode(kNatronTestPluginSequentialStatus);
    NodePtr node = NatronTestPlugin::createNode(kNatronTestPluginInOrder);
    // destroys the nodes at the end of the test, also when an ASSERT returns early
    const NodesGuard guard{ {node, source, generator} };
    ASSERT_TRUE(generator && source && node);
    ProjectPtr project = node->getApp()->getProject();
    project->connectNodes(0, generator, source);
    project->connectNodes(0, source, node);
    ASSERT_EQ( source, node->getInput(0) );
    // the frames of Source are cached, as they are when the node has several outputs
    KnobBoolPtr forceCaching = std::dynamic_pointer_cast<KnobBool>( source->getKnobByName("forceCaching") );
    ASSERT_TRUE( bool(forceCaching) );
    forceCaching->setValue(true);
    EXPECT_EQ( eSequentialPreferenceOnlySequential, node->getEffectInstance()->getSequentialPreference() );

    const int rendersBefore = NatronTestPlugin::counter("InOrder.renders");
    NatronTestPlugin::setCounter("InOrder.timeouts", 0);
    NatronTestPlugin::setCounter("InOrder.contended", 0);
    NatronTestPlugin::setCounter("InOrder.maxActive", 0);

    NatronTestPlugin::renderSequence(node, kFirstFrame, kLastFrame);

    EXPECT_EQ( kLastFrame - kFirstFrame + 1, NatronTestPlugin::counter("InOrder.renders") - rendersBefore );
    EXPECT_EQ( 0, NatronTestPlugin::counter("InOrder.timeouts") ) << "a frame waited for the previous one, which waited for "
                                                                      "the lock of an image that the first one held";
    // A pass checks something only if a frame held its source image while it waited for the previous one
    if ( (NatronTestPlugin::counter("InOrder.maxActive") < 2) || (NatronTestPlugin::counter("InOrder.contended") == 0) ) {
        GTEST_SKIP() << "the frames were not rendered in parallel, or no frame waited for the previous one: the case "
                        "was not built (one render thread?)";
    }
}
