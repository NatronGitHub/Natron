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

// kOfxImageEffectPropSequentialRenderStatus is 1 only when the host renders the effect in frame order on a
// single instance, and a plug-in that sets kOfxImageEffectInstancePropSequentialRender to 1 (on its
// descriptor or on its instance) is rendered that way when rendering to disk, without the frames it
// rendered before out of order (#1375). The renders on disk go through a DiskCache node, which is an
// output node that renders its input in background mode.

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <set>
#include <string>
#include <thread>
#include <vector>

#include <gtest/gtest.h>

#include "Engine/KnobTypes.h"
#include "Engine/ParallelRenderArgs.h"

#include "OfxTestPlugin.h"

NATRON_NAMESPACE_USING

namespace {
const int kFirstFrame = 1;
const int kLastFrame = 12;

struct Counters
{
    int sequentialRenders;
    int nonSequentialRenders;
    int outOfOrder;
};

Counters
getCounters(const std::string& id)
{
    Counters c;

    c.sequentialRenders = NatronTestPlugin::counter(id + ".sequentialRenders");
    c.nonSequentialRenders = NatronTestPlugin::counter(id + ".nonSequentialRenders");
    c.outOfOrder = NatronTestPlugin::counter(id + ".outOfOrder");

    return c;
}

// generator -> node (a test plug-in with the given ID) [-> downstream (another test plug-in)] -> DiskCache
struct Graph
{
    NodePtr generator, node, downstream, output;

    explicit Graph(const char* pluginID,
                   const char* downstreamPluginID = 0)
    {
        generator = NatronTestPlugin::createNode(kNatronTestPluginGenerator);
        node = NatronTestPlugin::createNode(pluginID);
        if (downstreamPluginID) {
            downstream = NatronTestPlugin::createNode(downstreamPluginID);
        }
        output = NatronTestPlugin::createNode(PLUGINID_NATRON_DISKCACHE);
        if ( generator && node && output && ( downstream || !downstreamPluginID ) ) {
            ProjectPtr project = node->getApp()->getProject();
            project->connectNodes(0, generator, node);
            if (downstream) {
                project->connectNodes(0, node, downstream);
                project->connectNodes(0, downstream, output);
            } else {
                project->connectNodes(0, node, output);
            }
        }
    }

    ~Graph()
    {
        NodePtr nodes[4] = { output, downstream, node, generator };
        for (int i = 0; i < 4; ++i) {
            if (nodes[i]) {
                nodes[i]->destroyNode(true, false);
            }
        }
    }

    bool isValid() const
    {
        return generator && node && output && node->getInput(0) == generator &&
               output->getInput(0) == (downstream ? downstream : node) && ( !downstream || downstream->getInput(0) == node );
    }
};

bool
canRunTest(const char* pluginID)
{
    if ( !NatronTestPlugin::isAvailable(pluginID) || !NatronTestPlugin::isAvailable(kNatronTestPluginGenerator) ) {
        NatronTestPlugin::reportSkipped(pluginID);

        return false;
    }

    return true;
}

// Render a few frames interactively (out of order, as when scrubbing the timeline), then the frame range
// on disk, and check that the plug-in was rendered in frame order on all the frames, without reusing
// the frames rendered interactively.
void
checkSequentialRender(const char* pluginID,
                      const std::string& id,
                      bool setOnInstance)
{
    Graph g(pluginID);
    ASSERT_TRUE( g.isValid() );
    if (setOnInstance) {
        // "kOfxImageEffectInstancePropSequentialRender [...] plugin instance (read/write)"
        KnobInt* knob = dynamic_cast<KnobInt*>( g.node->getKnobByName("instanceSequential").get() );
        ASSERT_TRUE(knob != NULL);
        knob->setValue(1);
    }
    EXPECT_EQ( eSequentialPreferenceOnlySequential, g.node->getEffectInstance()->getSequentialPreference() );

    const Counters before = getCounters(id);
    ASSERT_TRUE( bool( NatronTestPlugin::renderFrame(g.node, 7.) ) );
    ASSERT_TRUE( bool( NatronTestPlugin::renderFrame(g.node, 5.) ) );
    const Counters interactive = getCounters(id);
    EXPECT_EQ(before.nonSequentialRenders + 2, interactive.nonSequentialRenders) << "interactive renders are not sequential";
    EXPECT_EQ(before.sequentialRenders, interactive.sequentialRenders);

    NatronTestPlugin::setCounter(id + ".maxActiveRenders", 0);
    NatronTestPlugin::renderSequence(g.output, kFirstFrame, kLastFrame);
    const Counters after = getCounters(id);
    // every frame, including the ones rendered interactively, rendered again with the status 1
    EXPECT_EQ(interactive.sequentialRenders + (kLastFrame - kFirstFrame + 1), after.sequentialRenders);
    EXPECT_EQ(interactive.nonSequentialRenders, after.nonSequentialRenders);
    EXPECT_EQ(interactive.outOfOrder, after.outOfOrder) << "frames rendered out of order with kOfxImageEffectPropSequentialRenderStatus=1";
    // one frame at a time
    EXPECT_EQ( 1, NatronTestPlugin::counter(id + ".maxActiveRenders") );
}
}

// kOfxImageEffectInstancePropSequentialRender=1 on the descriptor
TEST(SequentialRender, SequentialPluginRenderedInFrameOrder)
{
    if ( !canRunTest(kNatronTestPluginSequential) ) {
        return;
    }
    checkSequentialRender(kNatronTestPluginSequential, "Sequential", false);
}

// kOfxImageEffectInstancePropSequentialRender=1 set by the plug-in on its instance
TEST(SequentialRender, InstanceValueIsHonoured)
{
    if ( !canRunTest(kNatronTestPluginSequentialStatus) ) {
        return;
    }
    checkSequentialRender(kNatronTestPluginSequentialStatus, "SequentialStatus", true);
}

// A plug-in that doesn't need sequential renders is rendered as before, in parallel, but then it gets
// kOfxImageEffectPropSequentialRenderStatus=0.
TEST(SequentialRender, StatusIsOneOnlyInFrameOrder)
{
    if ( !canRunTest(kNatronTestPluginSequentialStatus) ) {
        return;
    }
    const std::string id("SequentialStatus");
    Graph g(kNatronTestPluginSequentialStatus);
    ASSERT_TRUE( g.isValid() );
    EXPECT_EQ( eSequentialPreferenceNotSequential, g.node->getEffectInstance()->getSequentialPreference() );

    const Counters before = getCounters(id);
    NatronTestPlugin::setCounter(id + ".maxActiveRenders", 0);
    NatronTestPlugin::renderSequence(g.output, kFirstFrame, kLastFrame);
    const Counters after = getCounters(id);
    EXPECT_EQ( kLastFrame - kFirstFrame + 1, (after.sequentialRenders - before.sequentialRenders) +
               (after.nonSequentialRenders - before.nonSequentialRenders) );
    EXPECT_EQ(before.outOfOrder, after.outOfOrder) << "frames rendered out of order with kOfxImageEffectPropSequentialRenderStatus=1";
    if (NatronTestPlugin::counter(id + ".maxActiveRenders") > 1) {
        EXPECT_EQ(before.sequentialRenders, after.sequentialRenders) << "frames rendered in parallel with kOfxImageEffectPropSequentialRenderStatus=1";
    }
}

// The nodes downstream of the sequential node use the image of the frame that was rendered for them, from the input
// images of the render, without rendering the sequential node again for each clipGetImage.
TEST(SequentialRender, DownstreamNodeRendersSequentialNodeOnce)
{
    if ( !canRunTest(kNatronTestPluginSequential) || !NatronTestPlugin::isAvailable(kNatronTestPluginMultiPAR) ) {
        return;
    }
    const std::string id("Sequential");
    Graph g(kNatronTestPluginSequential, kNatronTestPluginMultiPAR);
    ASSERT_TRUE( g.isValid() );

    const Counters before = getCounters(id);
    const int downstreamRenders = NatronTestPlugin::counter("MultiPAR.render");
    NatronTestPlugin::renderSequence(g.output, kFirstFrame, kLastFrame);
    const Counters after = getCounters(id);
    EXPECT_EQ( kLastFrame - kFirstFrame + 1, NatronTestPlugin::counter("MultiPAR.render") - downstreamRenders );
    // once per frame
    EXPECT_EQ(before.sequentialRenders + (kLastFrame - kFirstFrame + 1), after.sequentialRenders);
    EXPECT_EQ(before.nonSequentialRenders, after.nonSequentialRenders);
    EXPECT_EQ(before.outOfOrder, after.outOfOrder);
}

// A temporal node downstream (frames t-1, t and t+1) gets the frames that the sequential node rendered earlier in the
// same render from the cache: the sequential node renders each frame once, in frame order.
TEST(SequentialRender, TemporalNodeDownstream)
{
    if ( !canRunTest(kNatronTestPluginSequential) || !NatronTestPlugin::isAvailable(kNatronTestPluginTimeBlur) ) {
        return;
    }
    const std::string id("Sequential");
    Graph g(kNatronTestPluginSequential, kNatronTestPluginTimeBlur);
    ASSERT_TRUE( g.isValid() );

    const Counters before = getCounters(id);
    const int downstreamRenders = NatronTestPlugin::counter("TimeBlur.render");
    const int fetchFailed = NatronTestPlugin::counter("TimeBlur.fetchFailed");
    NatronTestPlugin::renderSequence(g.output, kFirstFrame, kLastFrame);
    const Counters after = getCounters(id);
    EXPECT_EQ( kLastFrame - kFirstFrame + 1, NatronTestPlugin::counter("TimeBlur.render") - downstreamRenders );
    EXPECT_EQ( fetchFailed, NatronTestPlugin::counter("TimeBlur.fetchFailed") );
    // frames kFirstFrame-1 to kLastFrame+1, once each
    EXPECT_EQ(before.sequentialRenders + (kLastFrame - kFirstFrame + 3), after.sequentialRenders);
    EXPECT_EQ(before.nonSequentialRenders, after.nonSequentialRenders);
    EXPECT_EQ(before.outOfOrder, after.outOfOrder);
}

// Several threads rendering the same image at the same time in a frame-order render: exactly one claims its first
// render, and its claim action (removing the cache entries rendered out of order) is done before any other thread
// learns that the image is claimed, so that none of them uses an entry that is being removed.
TEST(FrameOrderRender, ClaimFirstRenderIsAtomic)
{
    const int nThreads = 8;
    const U64 keyHash = 1234;
    FrameOrderRender render( (std::set<const Node*>()) );
    std::mutex startMutex;
    std::condition_variable startCond;
    bool start = false;
    std::atomic<int> claims(0);
    std::atomic<int> onClaimCalls(0);
    std::atomic<bool> onClaimDone(false);
    std::atomic<int> sawClaimBeforeDone(0);
    std::vector<std::thread> threads;

    for (int i = 0; i < nThreads; ++i) {
        threads.push_back( std::thread([&]() {
            {
                std::unique_lock<std::mutex> l(startMutex);
                startCond.wait(l, [&]() { return start; });
            }
            const bool claimed = render.claimFirstRender(keyHash, [&]() {
                ++onClaimCalls;
                // the others are all waiting for the claim by now
                std::this_thread::sleep_for( std::chrono::milliseconds(50) );
                onClaimDone = true;
            });
            if (claimed) {
                ++claims;
            } else if ( !onClaimDone || !render.isClaimed(keyHash) ) {
                ++sawClaimBeforeDone;
            }
        }) );
    }
    {
        std::lock_guard<std::mutex> l(startMutex);
        start = true;
    }
    startCond.notify_all();
    for (std::size_t i = 0; i < threads.size(); ++i) {
        threads[i].join();
    }
    EXPECT_EQ(1, claims);
    EXPECT_EQ(1, onClaimCalls);
    EXPECT_EQ(0, sawClaimBeforeDone);

    // a failed render releases its claim, a rendered one keeps it
    EXPECT_TRUE( render.isClaimed(keyHash) );
    render.releaseClaim(keyHash);
    EXPECT_FALSE( render.isClaimed(keyHash) );
    EXPECT_TRUE( render.claimFirstRender(keyHash, std::function<void()>()) );
    render.setRendered(keyHash);
    render.releaseClaim(keyHash);
    EXPECT_TRUE( render.isClaimed(keyHash) );
    EXPECT_FALSE( render.claimFirstRender(keyHash, std::function<void()>()) );
}
