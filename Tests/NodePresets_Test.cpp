/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2023 The Natron developers
 * (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
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

// Regression tests for https://github.com/NatronGitHub/Natron/issues/1247
// ("Loading a preset on Roto, RotoPaint or Tracker adds shapes/tracks instead of replacing them, and Undo
// duplicates them").
//
// Importing presets (and undoing it) goes through Node::loadKnobs() on an existing node, which loads the
// RotoContext and TrackerContext of the serialization: the items of the serialization must replace the
// current ones.

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <iostream>
#include <list>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "Engine/AppInstance.h"
#include "Engine/AppManager.h"
#include "Engine/Bezier.h"
#include "Engine/CreateNodeArgs.h"
#include "Engine/Node.h"
#include "Engine/NodeSerialization.h"
#include "Engine/Plugin.h"
#include "Engine/Project.h"
#include "Engine/RotoContext.h"
#include "Engine/RotoLayer.h"
#include "Engine/TrackMarker.h"
#include "Engine/TrackerContext.h"

NATRON_NAMESPACE_USING

namespace {
bool
isPluginAvailable(const char* pluginID)
{
    try {
        Plugin* p = appPTR->getPluginBinary(QString::fromUtf8(pluginID), -1, -1, false);

        return p && p->getLibraryBinary();
    } catch (...) {
        return false;
    }
}

bool
skipIfMissing(const char* pluginID)
{
    if ( isPluginAvailable(pluginID) ) {
        return false;
    }
    // The bundled google-test predates GTEST_SKIP(): say clearly that the test did not run.
    // TODO: Convert to GTEST_SKIP() when gtest updated.
    const testing::TestInfo* info = testing::UnitTest::GetInstance()->current_test_info();
    std::cerr << "[  SKIPPED ] " << info->test_case_name() << '.' << info->name()
              << ": plug-in " << pluginID << " not found (needs openfx-misc in OFX_PLUGIN_PATH)" << std::endl;

    return true;
}

std::list<std::string>
shapeNames(const RotoContextPtr& context)
{
    std::list<std::string> names;
    std::list<RotoDrawableItemPtr> items = context->getCurvesByRenderOrder(false);

    for (std::list<RotoDrawableItemPtr>::iterator it = items.begin(); it != items.end(); ++it) {
        names.push_back( (*it)->getScriptName() );
    }

    return names;
}

std::list<std::string>
trackNames(const TrackerContextPtr& context)
{
    std::list<std::string> names;
    std::vector<TrackMarkerPtr> markers;

    context->getAllMarkers(&markers);
    for (std::size_t i = 0; i < markers.size(); ++i) {
        names.push_back( markers[i]->getScriptName_mt_safe() );
    }

    return names;
}

void
checkRotoLoadReplacesShapes(const char* pluginID)
{
    AppInstancePtr app = appPTR->getTopLevelInstance();
    CreateNodeArgs args( pluginID, app->getProject() );
    NodePtr node = app->createNode(args);
    ASSERT_TRUE( bool(node) );
    RotoContextPtr context = node->getRotoContext();
    ASSERT_TRUE( bool(context) );

    // State A: one shape in a sub-layer (the "preset")
    RotoLayerPtr layer = context->addLayer();
    ASSERT_TRUE( bool(layer) );
    BezierPtr shapeA = context->makeBezier(0., 0., kRotoBezierBaseName, 1., false);
    ASSERT_TRUE( bool(shapeA) );
    const std::string nameA = shapeA->getScriptName();
    NodeSerializationPtr stateA = std::make_shared<NodeSerialization>(node);

    // State B: the sub-layer and its shape are deleted, another shape is drawn
    context->removeItem(layer);
    BezierPtr shapeB = context->makeBezier(10., 10., kRotoBezierBaseName, 1., false);
    ASSERT_TRUE( bool(shapeB) );
    // the name of the deleted shape is free again: give this one a name of its own
    ASSERT_TRUE( shapeB->setScriptName("OtherBezier") );
    const std::string nameB = shapeB->getScriptName();
    ASSERT_NE(nameA, nameB);
    NodeSerializationPtr stateB = std::make_shared<NodeSerialization>(node);
    ASSERT_EQ( std::list<std::string>(1, nameB), shapeNames(context) );

    // Load the preset: the shapes of state A replace those of state B
    node->loadKnobs(*stateA);
    EXPECT_EQ( std::list<std::string>(1, nameA), shapeNames(context) );
    EXPECT_EQ( 2u, context->getLayers().size() ); // base layer + sub-layer

    // Undo (LoadNodePresetsCommand::undo() loads the previous state the same way)
    node->loadKnobs(*stateB);
    EXPECT_EQ( std::list<std::string>(1, nameB), shapeNames(context) );
    EXPECT_EQ( 1u, context->getLayers().size() );

    // Redo
    node->loadKnobs(*stateA);
    EXPECT_EQ( std::list<std::string>(1, nameA), shapeNames(context) );

    node->destroyNode(true /*blockingDestroy*/, false /*autoReconnect*/);
}
} // anonymous namespace

TEST(NodePresets, RotoLoadReplacesShapes)
{
    // Roto shapes are backed by OpenFX nodes (see RotoDrawableItem::createNodes())
    if ( skipIfMissing(PLUGINID_OFX_ROTO) || skipIfMissing(PLUGINID_OFX_MERGE) ) {
        return;
    }
    checkRotoLoadReplacesShapes(PLUGINID_NATRON_ROTO);
}

TEST(NodePresets, RotoPaintLoadReplacesShapes)
{
    if ( skipIfMissing(PLUGINID_OFX_ROTO) || skipIfMissing(PLUGINID_OFX_MERGE) ) {
        return;
    }
    checkRotoLoadReplacesShapes(PLUGINID_NATRON_ROTOPAINT);
}

TEST(NodePresets, TrackerLoadReplacesTracks)
{
    // The Tracker node creates internal Transform and CornerPin nodes
    if ( skipIfMissing(PLUGINID_OFX_TRANSFORM) || skipIfMissing(PLUGINID_OFX_CORNERPIN) ) {
        return;
    }
    AppInstancePtr app = appPTR->getTopLevelInstance();
    CreateNodeArgs args( PLUGINID_NATRON_TRACKER, app->getProject() );
    NodePtr node = app->createNode(args);
    ASSERT_TRUE( bool(node) );
    TrackerContextPtr context = node->getTrackerContext();
    ASSERT_TRUE( bool(context) );

    // State A: two tracks (the "preset")
    TrackMarkerPtr trackA1 = context->createMarker();
    TrackMarkerPtr trackA2 = context->createMarker();
    std::list<std::string> namesA;
    namesA.push_back( trackA1->getScriptName_mt_safe() );
    namesA.push_back( trackA2->getScriptName_mt_safe() );
    NodeSerializationPtr stateA = std::make_shared<NodeSerialization>(node);

    // State B: those tracks are deleted, another one is created
    context->removeMarker(trackA1);
    context->removeMarker(trackA2);
    TrackMarkerPtr trackB = context->createMarker();
    std::list<std::string> namesB( 1, trackB->getScriptName_mt_safe() );
    NodeSerializationPtr stateB = std::make_shared<NodeSerialization>(node);
    ASSERT_EQ( namesB, trackNames(context) );

    // Load the preset
    node->loadKnobs(*stateA);
    EXPECT_EQ( namesA, trackNames(context) );

    // Undo
    node->loadKnobs(*stateB);
    EXPECT_EQ( namesB, trackNames(context) );

    // Redo
    node->loadKnobs(*stateA);
    EXPECT_EQ( namesA, trackNames(context) );

    node->destroyNode(true /*blockingDestroy*/, false /*autoReconnect*/);
}
