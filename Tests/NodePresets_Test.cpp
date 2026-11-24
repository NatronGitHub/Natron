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
// current ones. Restoring a PyPlug from a project does the same on the children made by the PyPlug script
// (NodeCollectionSerialization::restoreFromSerialization() with createNodes == false). In both cases the
// node is already declared to Python: the loaded items must be declared, and the replaced ones removed.

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
#include "Engine/AppManager.h" // NATRON_PYTHON_NAMESPACE::interpretPythonScript
#include "Engine/Bezier.h"
#include "Engine/CreateNodeArgs.h"
#include "Engine/KnobTypes.h"
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

// Runs a Python script (made of asserts): it must not raise
void
checkPython(const std::string& script)
{
    std::string err, output;
    bool ok = NATRON_NAMESPACE::NATRON_PYTHON_NAMESPACE::interpretPythonScript(script, &err, &output);

    EXPECT_TRUE(ok) << script << err;
    EXPECT_EQ(std::string(), err) << script;
}

// The Python name of a node, e.g. "app1.Roto1"
std::string
pythonName(const NodePtr& node)
{
    return node->getApp()->getAppIDString() + "." + node->getFullyQualifiedName();
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
    const std::string fullNameB = shapeB->getFullyQualifiedName(); // e.g. "Layer1.OtherBezier"
    ASSERT_NE(nameA, nameB);
    NodeSerializationPtr stateB = std::make_shared<NodeSerialization>(node);
    ASSERT_EQ( std::list<std::string>(1, nameB), shapeNames(context) );

    // Load the preset: the shapes of state A replace those of state B
    node->loadKnobs(*stateA);
    EXPECT_EQ( std::list<std::string>(1, nameA), shapeNames(context) );
    EXPECT_EQ( 2u, context->getLayers().size() ); // base layer + sub-layer
    // and Python sees the loaded shape, not the replaced one
    RotoItemPtr loadedA = context->getItemByName(nameA);
    ASSERT_TRUE( bool(loadedA) );
    const std::string roto = pythonName(node) + ".roto";
    checkPython("assert " + roto + "." + loadedA->getFullyQualifiedName() + ".getScriptName() == '" + nameA + "'\n"
                "assert not hasattr(" + roto + "." + fullNameB.substr(0, fullNameB.find('.')) + ", '" + nameB + "')\n");

    // Undo (LoadNodePresetsCommand::undo() loads the previous state the same way)
    node->loadKnobs(*stateB);
    EXPECT_EQ( std::list<std::string>(1, nameB), shapeNames(context) );
    EXPECT_EQ( 1u, context->getLayers().size() );
    checkPython("assert " + roto + "." + fullNameB + ".getScriptName() == '" + nameB + "'\n");

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
    // and Python sees the loaded tracks
    const std::string tracker = pythonName(node) + ".tracker";
    checkPython("assert " + tracker + "." + namesA.front() + ".getScriptName() == '" + namesA.front() + "'\n"
                "assert " + tracker + "." + namesA.back() + ".getScriptName() == '" + namesA.back() + "'\n");

    // Undo
    node->loadKnobs(*stateB);
    EXPECT_EQ( namesB, trackNames(context) );

    // Redo
    node->loadKnobs(*stateA);
    EXPECT_EQ( namesA, trackNames(context) );

    node->destroyNode(true /*blockingDestroy*/, false /*autoReconnect*/);
}

// A pattern-matching track (the Tracker's "usePatternMatching") creates an internal TrackerPM node, which is
// obsolete (kOfxImageEffectPluginPropObsolete) and so not user-creatable: its creation was refused, and creating
// or loading such a track threw (#1513).
TEST(NodePresets, TrackerLoadPatternMatchingTracks)
{
    if ( skipIfMissing(PLUGINID_OFX_TRANSFORM) || skipIfMissing(PLUGINID_OFX_CORNERPIN) ||
         skipIfMissing(PLUGINID_OFX_TRACKERPM) ) {
        return;
    }
    AppInstancePtr app = appPTR->getTopLevelInstance();
    CreateNodeArgs args( PLUGINID_NATRON_TRACKER, app->getProject() );
    NodePtr node = app->createNode(args);
    ASSERT_TRUE( bool(node) );
    TrackerContextPtr context = node->getTrackerContext();
    ASSERT_TRUE( bool(context) );
    KnobBoolPtr usePM = std::dynamic_pointer_cast<KnobBool>( node->getKnobByName("usePatternMatching") );
    ASSERT_TRUE( bool(usePM) );
    usePM->setValue(true);
    ASSERT_TRUE( context->isTrackerPMEnabled() );

    TrackMarkerPtr track = context->createMarker();
    ASSERT_TRUE( bool(track) );
    EXPECT_TRUE( dynamic_cast<TrackMarkerPM*>( track.get() ) != NULL );
    const std::list<std::string> names( 1, track->getScriptName_mt_safe() );
    NodeSerializationPtr saved = std::make_shared<NodeSerialization>(node);

    // load it, as a project with that track does
    node->loadKnobs(*saved);
    EXPECT_EQ( names, trackNames(context) );
    std::vector<TrackMarkerPtr> markers;
    context->getAllMarkers(&markers);
    ASSERT_EQ( 1u, markers.size() );
    EXPECT_TRUE( dynamic_cast<TrackMarkerPM*>( markers[0].get() ) != NULL );

    node->destroyNode(true /*blockingDestroy*/, false /*autoReconnect*/);
}

// A PyPlug script makes a shape on a Roto node inside the PyPlug; the project saved the node with that
// shape. When the project is loaded, the script runs again (the node gets its shape), then the node is
// restored from the project with Node::loadKnobs(): the node must have the shape once, and Python must
// see it.
TEST(NodePresets, ScriptMadeRotoRestoredFromProject)
{
    if ( skipIfMissing(PLUGINID_OFX_ROTO) || skipIfMissing(PLUGINID_OFX_MERGE) ) {
        return;
    }
    AppInstancePtr app = appPTR->getTopLevelInstance();
    CreateNodeArgs args( PLUGINID_NATRON_ROTO, app->getProject() );
    NodePtr node = app->createNode(args);
    ASSERT_TRUE( bool(node) );
    RotoContextPtr context = node->getRotoContext();
    ASSERT_TRUE( bool(context) );
    const std::string roto = pythonName(node) + ".roto";

    // The "PyPlug script" makes the shape
    checkPython("b = " + roto + ".createBezier(0., 0., 1.)\n"
                "assert b is not None\n");
    std::list<std::string> names = shapeNames(context);
    ASSERT_EQ( 1u, names.size() );
    const std::string shapeName = names.front();
    NodeSerializationPtr saved = std::make_shared<NodeSerialization>(node);

    // The project restores the node
    node->loadKnobs(*saved);
    EXPECT_EQ( std::list<std::string>(1, shapeName), shapeNames(context) );
    RotoItemPtr loaded = context->getItemByName(shapeName);
    ASSERT_TRUE( bool(loaded) );
    checkPython("assert " + roto + "." + loaded->getFullyQualifiedName() + ".getScriptName() == '" + shapeName + "'\n");

    node->destroyNode(true /*blockingDestroy*/, false /*autoReconnect*/);
}
