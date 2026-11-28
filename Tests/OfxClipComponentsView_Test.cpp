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

// In a multi-view project, the getClipComponents action of a multi-planar OpenFX plug-in
// (kFnOfxImageEffectActionGetClipComponents, nuke/fnOfxExtensions.h) gets the view that is rendered in
// kFnOfxImageEffectPropView (NatronGitHub/openfx#331; it was always 0), and Natron fetches the planes
// that the plug-in passes through at the view it returns. The test plug-in net.natron.test.ClipComponentsView
// (Tests/OfxTestPlugin/NatronTestPluginClipComponentsView.cpp) counts the views it gets, and returns
// the view as the pass-through view, as the Support library's plug-ins do.

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <bitset>
#include <cstring>
#include <list>
#include <string>

#include <gtest/gtest.h>

#include <ofxCore.h>

#include "Engine/AppInstance.h"
#include "Engine/AppManager.h"
#include "Engine/EffectInstance.h"
#include "Engine/ImagePlaneDesc.h"
#include "Engine/KnobFile.h"
#include "Engine/Node.h"
#include "Engine/Project.h"

#include "OfxTestPlugin.h"

#define kClipComponentsViewPluginID "net.natron.test.ClipComponentsView"

NATRON_NAMESPACE_USING

// a stereo project, and back to the views the other tests expect (a single view) at the end, also
// if an assertion fails
class OfxClipComponentsView
    : public testing::Test
{
protected:

    virtual void SetUp() OVERRIDE FINAL
    {
        _app = appPTR->getTopLevelInstance();
        ASSERT_TRUE( bool(_app) );
        _viewsList = std::dynamic_pointer_cast<KnobPath>( _app->getProject()->getKnobByName("viewsList") );
        ASSERT_TRUE( bool(_viewsList) );
        _savedViewsList = _viewsList->getValue();
        _app->getProject()->setupProjectForStereo();
    }

    virtual void TearDown() OVERRIDE FINAL
    {
        if (_node) {
            _node->destroyNode(true /*blockingDestroy*/, false /*autoReconnect*/);
            _node.reset();
        }
        if (_viewsList) {
            _viewsList->setValue(_savedViewsList);
        }
    }

    AppInstancePtr _app;
    KnobPathPtr _viewsList;
    std::string _savedViewsList;
    NodePtr _node;
};

TEST_F(OfxClipComponentsView, ViewOfGetClipComponents)
{
    if ( !NatronTestPlugin::isAvailable(kClipComponentsViewPluginID) ) {
        NatronTestPlugin::reportMissing(kClipComponentsViewPluginID);

        return;
    }
    ASSERT_EQ(_app->getProject()->getProjectViewsCount(), 2);

    _node = NatronTestPlugin::createNode(kClipComponentsViewPluginID);
    ASSERT_TRUE(_node != 0);
    EffectInstancePtr effect = _node->getEffectInstance();
    ASSERT_TRUE( effect && effect->isMultiPlanar() );
    for (int view = 0; view < 2; ++view) {
        NatronTestPlugin::setCounter("ClipComponentsView.view" + std::to_string(view), 0);
    }
    for (int view = 0; view < 2; ++view) {
        EffectInstance::ComponentsNeededMap comps;
        std::list<ImagePlaneDesc> passThroughPlanes;
        bool processAll = false;
        double passThroughTime = 0.;
        int passThroughView = -1;
        std::bitset<4> processChannels;
        int passThroughInputNb = -1;
        // a hash per view that no other call used, so that the action isn't answered from the actions cache
        EXPECT_EQ( eStatusOK, effect->getComponentsNeededAndProduced_public(0x1742000 + view, 0., ViewIdx(view), &comps, &passThroughPlanes, &processAll,
                                                                            &passThroughTime, &passThroughView, &processChannels, &passThroughInputNb) );
        EXPECT_EQ( 1, NatronTestPlugin::counter("ClipComponentsView.view" + std::to_string(view)) ) << view;
        EXPECT_EQ(view, passThroughView);
        EXPECT_EQ(0, passThroughInputNb);
    }
    EXPECT_EQ( 0, NatronTestPlugin::counter("ClipComponentsView.view-1") );
}

namespace {
// the status of the getClipComponents action of net.natron.test.ClipComponentsView while the scope lives,
// and kOfxStatOK again after it, also if an assertion fails
class ClipComponentsStatusScope
{
public:
    explicit ClipComponentsStatusScope(OfxStatus status)
    {
        NatronTestPlugin::setCounter("ClipComponentsView.status", status);
    }

    ClipComponentsStatusScope(const ClipComponentsStatusScope&) = delete;
    ClipComponentsStatusScope& operator=(const ClipComponentsStatusScope&) = delete;

    ~ClipComponentsStatusScope()
    {
        NatronTestPlugin::setCounter("ClipComponentsView.status", kOfxStatOK);
    }
};

// the outputs of getComponentsNeededAndProduced_public(), set to values that no call returns
struct ComponentsNeeded
{
    EffectInstance::ComponentsNeededMap comps;
    std::list<ImagePlaneDesc> passThroughPlanes;
    bool processAll = true;
    double passThroughTime = -42.;
    int passThroughView = 42;
    std::bitset<4> processChannels;
    int passThroughInputNb = 42;

    StatusEnum get(const EffectInstancePtr& effect,
                   U64 hash,
                   double time,
                   int view)
    {
        return effect->getComponentsNeededAndProduced_public(hash, time, ViewIdx(view), &comps, &passThroughPlanes, &processAll,
                                                             &passThroughTime, &passThroughView, &processChannels, &passThroughInputNb);
    }
};
} // anonymous namespace

// Regression test for #2063 (https://github.com/NatronGitHub/Natron/issues/2063): the statuses of
// kFnOfxImageEffectActionGetClipComponents (nuke/fnOfxExtensions.h), which lists none, so that the generic
// rule applies: "The OfxStatus value returned is dependent upon the action being called; however the value
// kOfxStatReplyDefault is returned if the plug-in does not trap the action" (OFX 1.5.1, ofxCoreAPI.rst).
// Natron took every status but kOfxStatFailed for a success, with the pass-through time and view that
// HostSupport sets only on kOfxStatOK (uninitialized), and cached them; with kOfxStatFailed it cached the
// outputs as the caller left them. Now:
// - kOfxStatOK and kOfxStatReplyDefault succeed; with kOfxStatReplyDefault, the pass-through time and view
//   are those of the call (the defaults of HostSupport), with no pass-through input;
// - kOfxStatFailed gives the same defaults, which aren't cached (the next call calls the action again), and
//   the render goes on: the Support library sends kOfxStatReplyDefault when a plug-in fails before it sets
//   anything, and the MultiPlane plug-ins of openfx-misc fail when a channel that they may not render is
//   missing from their input;
// - the other error statuses fail: the outputs are left as they were, nothing is cached, and the render
//   fails with an error message, the one that a failed render action gets ("Out of memory!" for
//   kOfxStatErrMemory, "Unknown failure reason." otherwise; printed in background mode).
// Each case also renders the node: the cases that don't fail must render (a positive control).
TEST_F(OfxClipComponentsView, GetClipComponentsStatus)
{
    for ( const char* id : { kClipComponentsViewPluginID, kNatronTestPluginGenerator } ) {
        if ( !NatronTestPlugin::isAvailable(id) ) {
            NatronTestPlugin::reportMissing(id);

            return;
        }
    }
    struct Case
    {
        const char* id;
        OfxStatus status;
        StatusEnum expected;
        bool cached; // the result is cached
        int passThroughInputNb; // if expected is eStatusOK
        const char* message; // the error message of the render, if expected isn't eStatusOK
    };
    const Case cases[] = {
        { "OK", kOfxStatOK, eStatusOK, true, 0, nullptr },
        { "ReplyDefault", kOfxStatReplyDefault, eStatusOK, true, -1, nullptr },
        { "Failed", kOfxStatFailed, eStatusOK, false, -1, nullptr },
        { "ErrMemory", kOfxStatErrMemory, eStatusOutOfMemory, false, 0, "Out of memory!" },
        { "ErrFatal", kOfxStatErrFatal, eStatusFailed, false, 0, "Unknown failure reason." },
    };
    const double time = 3.;
    const int view = 1;
    U64 hash = 0x2063000; // a hash per case that no other call used, so that the actions cache doesn't answer

    for (const Case& c : cases) {
        NatronTest::NodeScope nodes;
        NodePtr generator = nodes.create(kNatronTestPluginGenerator);
        NodePtr node = nodes.create(kClipComponentsViewPluginID);
        ASSERT_TRUE( generator && node && _app->getProject()->connectNodes(0, generator, node) ) << c.id;
        EffectInstancePtr effect = node->getEffectInstance();
        const std::string viewCounter = "ClipComponentsView.view" + std::to_string(view);
        ++hash;
        ClipComponentsStatusScope status(c.status);
        {
            const int calls = NatronTestPlugin::counter(viewCounter);
            ComponentsNeeded needed;
            EXPECT_EQ( c.expected, needed.get(effect, hash, time, view) ) << c.id;
            EXPECT_EQ( 1, NatronTestPlugin::counter(viewCounter) - calls ) << c.id;
            if (c.expected == eStatusOK) {
                EXPECT_EQ(time, needed.passThroughTime) << c.id;
                EXPECT_EQ(view, needed.passThroughView) << c.id;
                EXPECT_EQ(c.passThroughInputNb, needed.passThroughInputNb) << c.id;
            } else {
                const ComponentsNeeded unset;
                EXPECT_TRUE( needed.comps.empty() ) << c.id;
                EXPECT_EQ(unset.passThroughTime, needed.passThroughTime) << c.id;
                EXPECT_EQ(unset.passThroughView, needed.passThroughView) << c.id;
                EXPECT_EQ(unset.passThroughInputNb, needed.passThroughInputNb) << c.id;
            }
        }
        {
            // the same call again: answered by the actions cache, or the action is called again
            const int calls = NatronTestPlugin::counter(viewCounter);
            ComponentsNeeded needed;
            EXPECT_EQ( c.expected, needed.get(effect, hash, time, view) ) << c.id;
            EXPECT_EQ( c.cached ? 0 : 1, NatronTestPlugin::counter(viewCounter) - calls ) << c.id;
        }
        // the render, at view 0 (in background mode, the error message is printed)
        testing::internal::CaptureStdout();
        const bool rendered = bool( NatronTestPlugin::renderFrame(node, time) );
        const std::string output = testing::internal::GetCapturedStdout();
        EXPECT_EQ(c.expected == eStatusOK, rendered) << c.id << ": " << output;
        for ( const char* message : { "Unknown failure reason.", "Out of memory!" } ) {
            const bool expectMessage = c.message && (std::strcmp(c.message, message) == 0);
            EXPECT_EQ( expectMessage, output.find(message) != std::string::npos ) << c.id << ": " << output;
        }
    }
}
