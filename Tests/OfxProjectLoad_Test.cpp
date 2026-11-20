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

// After a project load, when the inputs are connected and the clip preferences computed, Natron sends each OFX
// effect one kOfxActionInstanceChanged per input clip with an input node, with kOfxTypeClip and
// kOfxChangePluginEdited (#1107; openfx main, after 1.5.1, from #184/#191, asks for an instanceChanged then, with
// kOfxChangeUserEdited: see kProjectLoadChangeReason in Engine/OfxEffectInstance.cpp and
// https://github.com/AcademySoftwareFoundation/openfx/issues/294). The test plug-in net.natron.test.ProjectLoad
// (Tests/OfxTestPlugin/NatronTestPluginProjectLoad.cpp) counts these actions and, like a writer, sets parameters from
// its source clip when the user connects it: the values saved in the project must be the ones it has after the
// project is loaded.

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <map>
#include <string>

#include <QtCore/QTemporaryDir>

#include <gtest/gtest.h>

#include <ofxCore.h>
#include <ofxImageEffect.h>

#include "Engine/AppInstance.h"
#include "Engine/AppManager.h"
#include "Engine/CreateNodeArgs.h"
#include "Engine/EffectInstance.h"
#include "Engine/KnobTypes.h"
#include "Engine/Node.h"
#include "Engine/NodeGroup.h"
#include "Engine/Project.h"

#include "OfxTestPlugin.h"

#define kNatronTestPluginProjectLoad "net.natron.test.ProjectLoad"

NATRON_NAMESPACE_USING

namespace {
const char* const kCounters[] = {
    "ProjectLoad.created",
    "ProjectLoad.begin." kOfxChangeUserEdited, "ProjectLoad.begin." kOfxChangePluginEdited,
    "ProjectLoad.end." kOfxChangeUserEdited, "ProjectLoad.end." kOfxChangePluginEdited,
    "ProjectLoad.clip." kOfxImageEffectSimpleSourceClipName "." kOfxChangeUserEdited,
    "ProjectLoad.clip." kOfxImageEffectSimpleSourceClipName "." kOfxChangePluginEdited,
    "ProjectLoad.clip.Mask." kOfxChangeUserEdited, "ProjectLoad.clip.Mask." kOfxChangePluginEdited,
    "ProjectLoad.clip." kOfxImageEffectOutputClipName "." kOfxChangeUserEdited,
    "ProjectLoad.clip." kOfxImageEffectOutputClipName "." kOfxChangePluginEdited,
    "ProjectLoad.param." kOfxChangeUserEdited, "ProjectLoad.param." kOfxChangePluginEdited,
    "ProjectLoad.connected", "ProjectLoad.rod", "ProjectLoad.sourcePAR100", "ProjectLoad.sourceFrameRate",
    "ProjectLoad.error.outsideBracket", "ProjectLoad.error.reasonMismatch",
};

std::map<std::string, int>
readCounters()
{
    std::map<std::string, int> values;

    for (std::size_t i = 0; i < sizeof(kCounters) / sizeof(kCounters[0]); ++i) {
        values[kCounters[i]] = NatronTestPlugin::counter(kCounters[i]);
    }

    return values;
}

// the counters that changed since start
std::map<std::string, int>
countersSince(const std::map<std::string, int>& start)
{
    std::map<std::string, int> delta;
    const std::map<std::string, int> now = readCounters();

    for (std::map<std::string, int>::const_iterator it = now.begin(); it != now.end(); ++it) {
        std::map<std::string, int>::const_iterator found = start.find(it->first);
        const int d = it->second - ( found == start.end() ? 0 : found->second );
        if (d != 0) {
            delta[it->first] = d;
        }
    }

    return delta;
}

int
inputNamed(const NodePtr& node,
           const std::string& label)
{
    for (int i = 0; i < node->getNInputs(); ++i) {
        if (node->getInputLabel(i) == label) {
            return i;
        }
    }

    return -1;
}

bool
pluginsAvailable()
{
    if ( !NatronTestPlugin::isAvailable(kNatronTestPluginProjectLoad) ||
         !NatronTestPlugin::isAvailable(kNatronTestPluginGenerator) ) {
        NatronTestPlugin::reportSkipped(kNatronTestPluginProjectLoad);

        return false;
    }

    return true;
}

template <typename KNOB>
std::shared_ptr<KNOB>
knob(const NodePtr& node,
     const std::string& name)
{
    return std::dynamic_pointer_cast<KNOB>( node->getKnobByName(name) );
}
} // anonymous namespace

// A project with a source connected to the plug-in, whose parameters the user changed after connecting it: after the
// load, the plug-in gets exactly one kOfxActionInstanceChanged, on the connected Source clip, with
// kOfxChangePluginEdited, in a bracket, with a working clipGetRegionOfDefinition and the clip preferences of the source
// (its pixel aspect ratio); a plug-in without connected inputs gets none; every value is the one that was saved, and
// the node keeps its age (hence its hash).
TEST(OfxProjectLoad, InstanceChangedAfterLoadKeepsSavedValues)
{
    if ( !pluginsAvailable() ) {
        return;
    }
    QTemporaryDir tmpDir;
    ASSERT_TRUE( tmpDir.isValid() );
    AppInstancePtr app = appPTR->getTopLevelInstance();
    ProjectPtr project = app->getProject();

    NodePtr generator = NatronTestPlugin::createNode(kNatronTestPluginGenerator);
    NodePtr connected = NatronTestPlugin::createNode(kNatronTestPluginProjectLoad);
    NodePtr alone = NatronTestPlugin::createNode(kNatronTestPluginProjectLoad);
    ASSERT_TRUE(generator && connected && alone);
    const int source = inputNamed(connected, kOfxImageEffectSimpleSourceClipName);
    ASSERT_GE(source, 0);

    // the user connects the source: the plug-in sets premult and frameRate from it
    std::map<std::string, int> start = readCounters();
    ASSERT_TRUE( connected->connectInput(generator, source) );
    std::map<std::string, int> delta = countersSince(start);
    EXPECT_EQ( 1, delta["ProjectLoad.clip." kOfxImageEffectSimpleSourceClipName "." kOfxChangeUserEdited] );
    EXPECT_EQ( 1, delta["ProjectLoad.rod"] );
    EXPECT_TRUE( knob<KnobBool>(connected, "premult")->getValue() );
    EXPECT_EQ( 50., knob<KnobDouble>(connected, "frameRate")->getValue() );

    // then changes them, and saves the project
    knob<KnobBool>(connected, "premult")->setValue(false);
    knob<KnobDouble>(connected, "frameRate")->setValue(12.5);
    knob<KnobString>(connected, "file")->setValue("/path/to/out_####.exr");
    knob<KnobDouble>(alone, "frameRate")->setValue(30.);
    knob<KnobDouble>(generator, "par")->setValue(2.);
    const U64 savedAge = connected->getKnobsAge();
    const double generatorFrameRate = generator->getEffectInstance()->getFrameRate();
    const std::string connectedName = connected->getScriptName();
    const std::string aloneName = alone->getScriptName();
    const QString dir = tmpDir.path() + QLatin1Char('/');
    QString savedPath;
    ASSERT_TRUE( project->saveProject( dir, QString::fromUtf8("ProjectLoad.ntp"), &savedPath ) );
    generator.reset();
    connected.reset();
    alone.reset();

    start = readCounters();
    testing::internal::CaptureStdout();
    const bool loaded = project->loadProject( dir, QString::fromUtf8("ProjectLoad.ntp") );
    const std::string output = testing::internal::GetCapturedStdout();
    ASSERT_TRUE(loaded);
    delta = countersSince(start);
    // no value changed: not modified, nothing logged
    EXPECT_TRUE( project->isSaveUpToDate() );
    EXPECT_EQ( std::string::npos, output.find("Project load: plug-ins changed parameter values") ) << output;

    std::map<std::string, int> expected;
    expected["ProjectLoad.created"] = 2;
    expected["ProjectLoad.begin." kOfxChangePluginEdited] = 1;
    expected["ProjectLoad.end." kOfxChangePluginEdited] = 1;
    expected["ProjectLoad.clip." kOfxImageEffectSimpleSourceClipName "." kOfxChangePluginEdited] = 1;
    expected["ProjectLoad.connected"] = 1;
    expected["ProjectLoad.rod"] = 1;
    expected["ProjectLoad.sourcePAR100"] = 200;
    expected["ProjectLoad.sourceFrameRate"] = (int)( generatorFrameRate + 0.5 );
    EXPECT_EQ(expected, delta);

    connected = project->getNodeByName(connectedName);
    alone = project->getNodeByName(aloneName);
    ASSERT_TRUE(connected && alone);
    EXPECT_TRUE( bool( connected->getInput(source) ) );
    EXPECT_FALSE( knob<KnobBool>(connected, "premult")->getValue() );
    EXPECT_EQ( 12.5, knob<KnobDouble>(connected, "frameRate")->getValue() );
    EXPECT_EQ( std::string("/path/to/out_####.exr"), knob<KnobString>(connected, "file")->getValue() );
    EXPECT_FALSE( knob<KnobBool>(alone, "premult")->getValue() );
    EXPECT_EQ( 30., knob<KnobDouble>(alone, "frameRate")->getValue() );
    EXPECT_EQ( savedAge, connected->getKnobsAge() );
    connected.reset();
    alone.reset();

    // leave the shared app with an untitled empty project, before the directory is removed
    project->resetProject();
}

// Like AppendClip's "lastFrame", the plug-in sets a value derived from its source in the action, whatever the reason:
// the saved value was stale (the user, or a shorter input). After the load, the project is marked modified and the
// changed nodes are logged. The node has a new age (hence hash), and
// its clip preferences, which read that value, and those of the node downstream, are computed again. The downstream
// node was created first, but its action comes after the one of its input, and sees the new metadata; its hash
// changes. A node that follows the value through a link, but is not downstream, also gets a new age.
TEST(OfxProjectLoad, ValueChangedAfterLoadRefreshesMetadataAndHash)
{
    if ( !pluginsAvailable() ) {
        return;
    }
    QTemporaryDir tmpDir;
    ASSERT_TRUE( tmpDir.isValid() );
    AppInstancePtr app = appPTR->getTopLevelInstance();
    ProjectPtr project = app->getProject();

    // created downstream first, so that the order of the project is not the order of the graph
    NodePtr downstream = NatronTestPlugin::createNode(kNatronTestPluginProjectLoad);
    NodePtr follower = NatronTestPlugin::createNode(kNatronTestPluginProjectLoad);
    NodePtr generator = NatronTestPlugin::createNode(kNatronTestPluginGenerator);
    NodePtr deriving = NatronTestPlugin::createNode(kNatronTestPluginProjectLoad);
    ASSERT_TRUE(generator && deriving && downstream && follower);
    const int source = inputNamed(deriving, kOfxImageEffectSimpleSourceClipName);
    ASSERT_GE(source, 0);
    knob<KnobBool>(deriving, "setDerived")->setValue(true);
    ASSERT_TRUE( deriving->connectInput(generator, source) );
    ASSERT_TRUE( downstream->connectInput(deriving, source) );
    const double height = knob<KnobDouble>(deriving, "derived")->getValue();
    ASSERT_GT(height, 7.);

    // a stale value is saved; the follower's frameRate is linked to it
    knob<KnobDouble>(deriving, "derived")->setValue(7.);
    ASSERT_TRUE( knob<KnobDouble>(follower, "frameRate")->slaveTo( 0, deriving->getKnobByName("derived"), 0 ) );
    const U64 savedAge = deriving->getKnobsAge();
    const U64 savedFollowerAge = follower->getKnobsAge();
    const U64 savedDownstreamHash = downstream->getHashValue();
    const std::string derivingName = deriving->getScriptName();
    const std::string downstreamName = downstream->getScriptName();
    const std::string followerName = follower->getScriptName();
    const QString dir = tmpDir.path() + QLatin1Char('/');
    QString savedPath;
    ASSERT_TRUE( project->saveProject( dir, QString::fromUtf8("ProjectLoadDerived.ntp"), &savedPath ) );
    generator.reset();
    deriving.reset();
    downstream.reset();
    follower.reset();

    const std::map<std::string, int> start = readCounters();
    testing::internal::CaptureStdout();
    const bool loaded = project->loadProject( dir, QString::fromUtf8("ProjectLoadDerived.ntp") );
    const std::string output = testing::internal::GetCapturedStdout();
    ASSERT_TRUE(loaded);
    std::map<std::string, int> delta = countersSince(start);
    // the values are not the saved ones: the project is modified, and the changed nodes are logged (stdout in
    // background mode, the Script Editor in the GUI), the follower too, but not the node downstream
    EXPECT_FALSE( project->isSaveUpToDate() );
    const std::size_t logged = output.find("Project load: plug-ins changed parameter values in: ");
    ASSERT_NE(std::string::npos, logged) << output;
    const std::string line = output.substr( logged, output.find('\n', logged) - logged );
    EXPECT_NE( std::string::npos, line.find(derivingName) ) << line;
    EXPECT_NE( std::string::npos, line.find(followerName) ) << line;
    EXPECT_EQ( std::string::npos, line.find(downstreamName) ) << line;
    EXPECT_NE( std::string::npos, line.find("(the project is marked modified)") ) << line;
    deriving = project->getNodeByName(derivingName);
    downstream = project->getNodeByName(downstreamName);
    follower = project->getNodeByName(followerName);
    ASSERT_TRUE(deriving && downstream && follower);
    EXPECT_EQ( height, knob<KnobDouble>(deriving, "derived")->getValue() );
    EXPECT_NE( savedAge, deriving->getKnobsAge() );
    EXPECT_EQ( height, deriving->getEffectInstance()->getFrameRate() );
    EXPECT_EQ( height, downstream->getEffectInstance()->getFrameRate() );
    EXPECT_NE( savedDownstreamHash, downstream->getHashValue() );
    // the actions of deriving (Source: the generator) and of downstream (Source: deriving, already refreshed)
    EXPECT_EQ( 2, delta["ProjectLoad.clip." kOfxImageEffectSimpleSourceClipName "." kOfxChangePluginEdited] );
    EXPECT_EQ( (int)( deriving->getInput(source)->getEffectInstance()->getFrameRate() + 0.5 ) + (int)(height + 0.5),
               delta["ProjectLoad.sourceFrameRate"] );
    EXPECT_EQ( height, knob<KnobDouble>(follower, "frameRate")->getValue() );
    EXPECT_NE( savedFollowerAge, follower->getKnobsAge() );
    deriving.reset();
    downstream.reset();
    follower.reset();

    project->resetProject();
}

// A node inside a Group, connected to the GroupInput, gets the action too, with the RoD of the source outside the
// group; and so does a project loaded as an auto-save (the path of Project::loadProject() for a recovered project).
TEST(OfxProjectLoad, NodeInsideGroupAndAutoSaveLoad)
{
    if ( !pluginsAvailable() ) {
        return;
    }
    QTemporaryDir tmpDir;
    ASSERT_TRUE( tmpDir.isValid() );
    AppInstancePtr app = appPTR->getTopLevelInstance();
    ProjectPtr project = app->getProject();

    NodePtr generator = NatronTestPlugin::createNode(kNatronTestPluginGenerator);
    CreateNodeArgs groupArgs( PLUGINID_NATRON_GROUP, project );
    groupArgs.setProperty<bool>(kCreateNodeArgsPropNodeGroupDisableCreateInitialNodes, true);
    groupArgs.setProperty<bool>(kCreateNodeArgsPropAddUndoRedoCommand, false);
    groupArgs.setProperty<bool>(kCreateNodeArgsPropAutoConnect, false);
    NodePtr groupNode = app->createNode(groupArgs);
    ASSERT_TRUE(generator && groupNode);
    NodeGroupPtr group = std::dynamic_pointer_cast<NodeGroup>( groupNode->getEffectInstance() );
    ASSERT_TRUE( bool(group) );
    NodePtr inside[2];
    const char* insideIDs[2] = { PLUGINID_NATRON_INPUT, kNatronTestPluginProjectLoad };
    for (int i = 0; i < 2; ++i) {
        CreateNodeArgs args(insideIDs[i], group);
        args.setProperty<bool>(kCreateNodeArgsPropAddUndoRedoCommand, false);
        args.setProperty<bool>(kCreateNodeArgsPropAutoConnect, false);
        inside[i] = app->createNode(args);
        ASSERT_TRUE( bool(inside[i]) );
    }
    const int source = inputNamed(inside[1], kOfxImageEffectSimpleSourceClipName);
    ASSERT_GE(source, 0);
    ASSERT_TRUE( inside[1]->connectInput(inside[0], source) );
    ASSERT_TRUE( groupNode->connectInput(generator, 0) );
    knob<KnobDouble>(inside[1], "frameRate")->setValue(12.5);
    const QString dir = tmpDir.path() + QLatin1Char('/');
    QString savedPath;
    ASSERT_TRUE( project->saveProject( dir, QString::fromUtf8("ProjectLoadGroup.ntp"), &savedPath ) );
    generator.reset();
    groupNode.reset();
    group.reset();
    inside[0].reset();
    inside[1].reset();

    for (int autoSave = 0; autoSave < 2; ++autoSave) {
        const std::map<std::string, int> start = readCounters();
        ASSERT_TRUE( project->loadProject( dir, QString::fromUtf8("ProjectLoadGroup.ntp"), autoSave != 0 ) );
        std::map<std::string, int> delta = countersSince(start);
        EXPECT_EQ( 1, delta["ProjectLoad.clip." kOfxImageEffectSimpleSourceClipName "." kOfxChangePluginEdited] ) << "autoSave " << autoSave;
        EXPECT_EQ( 0, delta["ProjectLoad.clip." kOfxImageEffectSimpleSourceClipName "." kOfxChangeUserEdited] ) << "autoSave " << autoSave;
        EXPECT_EQ( 1, delta["ProjectLoad.rod"] ) << "autoSave " << autoSave;
        EXPECT_EQ( 0, delta["ProjectLoad.error.outsideBracket"] + delta["ProjectLoad.error.reasonMismatch"] );
    }

    project->resetProject();
}

