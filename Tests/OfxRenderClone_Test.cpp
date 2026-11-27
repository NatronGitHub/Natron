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

// When two renders of an instance-safe plug-in run at the same time, Natron renders the second one with a
// render clone of the effect (EffectInstance::getOrCreateRenderInstance()), and keeps the clone for the
// next renders. The clones share the parameter and clip instances of their main instance. They were
// destroyed by ~EffectInstance() of the main instance, after ~OfxEffectInstance() had destroyed its
// parameters, so that the destructor of a clone went through dangling parameters: Natron crashed on quit
// in OfxEffectInstance::destroyInteracts() (#1801).
// A render clone is a new instance of the plug-in: in its kOfxActionCreateInstance, kOfxPropInstanceData is
// NULL (its default, "unique to each plug-in instance", ofxCore.h), not the main instance's pointer (#1385).
// A render clone reports the extra planes of its node (kNatronOfxExtraCreatedPlanes: the project's default planes
// and the node's user-created ones), as the main instance does: the property is computed by a get hook, which the
// copied property set didn't have (#2023).

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <gtest/gtest.h>

#include <chrono>
#include <thread>

#include <QtCore/QElapsedTimer>

#include "Engine/AppInstance.h"
#include "Engine/AppManager.h"
#include "Engine/Node.h"
#include "Engine/Project.h"

#include "OfxTestPlugin.h"

#define kNatronTestPluginRenderClone "net.natron.test.RenderClone"

NATRON_NAMESPACE_USING

namespace {
// Wait (5 s at most) until the counter of the test plug-in has the value
bool
waitForCounter(const char* name,
               int value)
{
    QElapsedTimer timer;

    timer.start();
    while ( (NatronTestPlugin::counter(name) != value) && (timer.elapsed() < 5000) ) {
        std::this_thread::sleep_for( std::chrono::milliseconds(1) );
    }

    return NatronTestPlugin::counter(name) == value;
}
} // anonymous namespace

TEST(OfxRenderClone, DestroyedBeforeMainInstance)
{
    if ( !NatronTestPlugin::isAvailable(kNatronTestPluginRenderClone) ) {
        NatronTestPlugin::reportMissing(kNatronTestPluginRenderClone);

        return;
    }
    const int created = NatronTestPlugin::counter("RenderClone.created");
    const int destroyedBeforeLater = NatronTestPlugin::counter("RenderClone.destroyedBeforeLater");
    const int instanceDataAtCreate = NatronTestPlugin::counter("RenderClone.instanceDataAtCreate");
    const int renderWithExtraPlanes = NatronTestPlugin::counter("RenderClone.renderWithExtraPlanes");
    const int renderWithoutExtraPlanes = NatronTestPlugin::counter("RenderClone.renderWithoutExtraPlanes");
    ASSERT_EQ( 0, NatronTestPlugin::counter("RenderClone.live") );

    NodePtr node = NatronTestPlugin::createNode(kNatronTestPluginRenderClone);
    ASSERT_TRUE( bool(node) );
    ASSERT_EQ( created + 1, NatronTestPlugin::counter("RenderClone.created") );

    // A render of frame 1 on the main instance blocks in the plug-in; meanwhile, a render of frame 2 needs a
    // render clone.
    NatronTestPlugin::setCounter("RenderClone.blockRender", 1);
    bool rendered1 = false;
    bool rendered2 = false;
    std::thread render1([&node, &rendered1]() {
        rendered1 = bool( NatronTestPlugin::renderFrame(node, 1.) );
    });
    EXPECT_TRUE( waitForCounter("RenderClone.inRender", 1) );
    std::thread render2([&node, &rendered2]() {
        rendered2 = bool( NatronTestPlugin::renderFrame(node, 2.) );
    });
    EXPECT_TRUE( waitForCounter("RenderClone.created", created + 2) );
    NatronTestPlugin::setCounter("RenderClone.blockRender", 0);
    render1.join();
    render2.join();
    EXPECT_TRUE(rendered1);
    EXPECT_TRUE(rendered2);
    // the main instance and its clone, which the effect keeps for the next renders
    EXPECT_EQ( 2, NatronTestPlugin::counter("RenderClone.live") );
    // the clone started with no instance data, although the main instance had set its own
    EXPECT_EQ( instanceDataAtCreate, NatronTestPlugin::counter("RenderClone.instanceDataAtCreate") );
    // both renders, on the main instance and on the clone, saw the extra planes (the project has five by default)
    EXPECT_EQ( renderWithExtraPlanes + 2, NatronTestPlugin::counter("RenderClone.renderWithExtraPlanes") );
    EXPECT_EQ( renderWithoutExtraPlanes, NatronTestPlugin::counter("RenderClone.renderWithoutExtraPlanes") );

    // as on quit (Project::reset(), Node::doDestroyNodeInternalEnd()), where the crash was seen; this also
    // destroys any node that an earlier test left in the project, as DeepGraphRender_Test.cpp does
    node.reset();
    appPTR->getTopLevelInstance()->getProject()->reset(false /*aboutToQuit*/, true /*blocking*/);
    EXPECT_EQ( 0, NatronTestPlugin::counter("RenderClone.live") );
    EXPECT_EQ( destroyedBeforeLater, NatronTestPlugin::counter("RenderClone.destroyedBeforeLater") );
}
