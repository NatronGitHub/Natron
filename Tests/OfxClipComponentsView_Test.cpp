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
#include <list>
#include <string>

#include <gtest/gtest.h>

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
        effect->getComponentsNeededAndProduced_public(0x1742000 + view, 0., ViewIdx(view), &comps, &passThroughPlanes, &processAll,
                                                      &passThroughTime, &passThroughView, &processChannels, &passThroughInputNb);
        EXPECT_EQ( 1, NatronTestPlugin::counter("ClipComponentsView.view" + std::to_string(view)) ) << view;
        EXPECT_EQ(view, passThroughView);
        EXPECT_EQ(0, passThroughInputNb);
    }
    EXPECT_EQ( 0, NatronTestPlugin::counter("ClipComponentsView.view-1") );
}
