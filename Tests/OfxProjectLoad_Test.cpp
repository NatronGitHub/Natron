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

#include <QtCore/QFile>
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
#include "Engine/Plugin.h"
#include "Engine/Project.h"
#include "Engine/Settings.h"

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

// the "Project load: ..." line of output that starts with prefix, or an empty string
std::string
loggedLine(const std::string& output,
           const std::string& prefix)
{
    const std::size_t logged = output.find(prefix);

    if (logged == std::string::npos) {
        return std::string();
    }

    return output.substr( logged, output.find('\n', logged) - logged );
}

#define kChangedLog "Project load: plug-ins changed parameter values in: "
#define kAnimatedLog "Project load: plug-ins tried to change animated parameters, which keep their animation, in: "

QByteArray
readFile(const QString& path)
{
    QFile file(path);

    if ( !file.open(QIODevice::ReadOnly) ) {
        return QByteArray();
    }

    return file.readAll();
}

bool
writeFile(const QString& path,
          const QByteArray& contents)
{
    QFile file(path);

    return file.open(QIODevice::WriteOnly | QIODevice::Truncate) && (file.write(contents) == contents.size());
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
// changes. The nodes that follow the value through a link or an expression, but are not downstream, also get a new
// age.
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
    NodePtr exprFollower = NatronTestPlugin::createNode(kNatronTestPluginProjectLoad);
    NodePtr generator = NatronTestPlugin::createNode(kNatronTestPluginGenerator);
    NodePtr deriving = NatronTestPlugin::createNode(kNatronTestPluginProjectLoad);
    ASSERT_TRUE(generator && deriving && downstream && follower && exprFollower);
    const int source = inputNamed(deriving, kOfxImageEffectSimpleSourceClipName);
    ASSERT_GE(source, 0);
    knob<KnobBool>(deriving, "setDerived")->setValue(true);
    ASSERT_TRUE( deriving->connectInput(generator, source) );
    ASSERT_TRUE( downstream->connectInput(deriving, source) );
    const double height = knob<KnobDouble>(deriving, "derived")->getValue();
    ASSERT_GT(height, 7.);

    // a stale value is saved; the follower's frameRate is linked to it, the exprFollower's follows it by an expression
    knob<KnobDouble>(deriving, "derived")->setValue(7.);
    ASSERT_TRUE( knob<KnobDouble>(follower, "frameRate")->slaveTo( 0, deriving->getKnobByName("derived"), 0 ) );
    knob<KnobDouble>(exprFollower, "frameRate")->setExpression(0, deriving->getScriptName() + ".derived.get()", false /*hasRetVariable*/, true /*failIfInvalid*/);
    ASSERT_EQ( 7., knob<KnobDouble>(exprFollower, "frameRate")->getValue() );
    const U64 savedAge = deriving->getKnobsAge();
    const U64 savedFollowerAge = follower->getKnobsAge();
    const U64 savedExprFollowerAge = exprFollower->getKnobsAge();
    const U64 savedDownstreamHash = downstream->getHashValue();
    const std::string derivingName = deriving->getScriptName();
    const std::string downstreamName = downstream->getScriptName();
    const std::string followerName = follower->getScriptName();
    const std::string exprFollowerName = exprFollower->getScriptName();
    const QString dir = tmpDir.path() + QLatin1Char('/');
    QString savedPath;
    ASSERT_TRUE( project->saveProject( dir, QString::fromUtf8("ProjectLoadDerived.ntp"), &savedPath ) );
    generator.reset();
    deriving.reset();
    downstream.reset();
    follower.reset();
    exprFollower.reset();

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
    EXPECT_NE( std::string::npos, line.find(exprFollowerName) ) << line;
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
    NodePtr exprFollowerLoaded = project->getNodeByName(exprFollowerName);
    ASSERT_TRUE( bool(exprFollowerLoaded) );
    EXPECT_EQ( height, knob<KnobDouble>(exprFollowerLoaded, "frameRate")->getValue() );
    EXPECT_NE( savedExprFollowerAge, exprFollowerLoaded->getKnobsAge() );
    exprFollowerLoaded.reset();
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


// Like WriteFFmpeg with its info parameters, the plug-in sets a display-only value (neither saved nor rendered) from
// its source in the action: the value is set, but the project is not modified, nothing is logged, and neither the
// node nor the node downstream gets a new age or hash (their cached images stay valid).
TEST(OfxProjectLoad, DisplayOnlyValueChangedAfterLoadKeepsProjectUnmodified)
{
    if ( !pluginsAvailable() ) {
        return;
    }
    QTemporaryDir tmpDir;
    ASSERT_TRUE( tmpDir.isValid() );
    AppInstancePtr app = appPTR->getTopLevelInstance();
    ProjectPtr project = app->getProject();

    NodePtr generator = NatronTestPlugin::createNode(kNatronTestPluginGenerator);
    NodePtr informing = NatronTestPlugin::createNode(kNatronTestPluginProjectLoad);
    NodePtr downstream = NatronTestPlugin::createNode(kNatronTestPluginProjectLoad);
    ASSERT_TRUE(generator && informing && downstream);
    const int source = inputNamed(informing, kOfxImageEffectSimpleSourceClipName);
    ASSERT_GE(source, 0);
    knob<KnobBool>(informing, "setInfo")->setValue(true);
    ASSERT_TRUE( informing->connectInput(generator, source) );
    ASSERT_TRUE( downstream->connectInput(informing, source) );
    const double width = knob<KnobDouble>(informing, "info")->getValue();
    ASSERT_GT(width, 0.);
    ASSERT_FALSE( informing->getKnobByName("info")->getIsPersistent() );
    ASSERT_FALSE( informing->getKnobByName("info")->getEvaluateOnChange() );

    const U64 savedAge = informing->getKnobsAge();
    const U64 savedHash = informing->getHashValue();
    const U64 savedDownstreamHash = downstream->getHashValue();
    const std::string informingName = informing->getScriptName();
    const std::string downstreamName = downstream->getScriptName();
    const QString dir = tmpDir.path() + QLatin1Char('/');
    QString savedPath;
    ASSERT_TRUE( project->saveProject( dir, QString::fromUtf8("ProjectLoadInfo.ntp"), &savedPath ) );
    generator.reset();
    informing.reset();
    downstream.reset();

    testing::internal::CaptureStdout();
    const bool loaded = project->loadProject( dir, QString::fromUtf8("ProjectLoadInfo.ntp") );
    const std::string output = testing::internal::GetCapturedStdout();
    ASSERT_TRUE(loaded);
    informing = project->getNodeByName(informingName);
    downstream = project->getNodeByName(downstreamName);
    ASSERT_TRUE(informing && downstream);
    // the value is not saved: the plug-in set it again in the action
    EXPECT_EQ( width, knob<KnobDouble>(informing, "info")->getValue() );
    EXPECT_TRUE( project->isSaveUpToDate() );
    EXPECT_EQ( std::string::npos, output.find("Project load: plug-ins") ) << output;
    EXPECT_EQ( savedAge, informing->getKnobsAge() );
    EXPECT_EQ( savedHash, informing->getHashValue() );
    EXPECT_EQ( savedDownstreamHash, downstream->getHashValue() );
    informing.reset();
    downstream.reset();

    project->resetProject();
}

// The plug-in sets its saved value "derived" in the action, but the user animated it: no keyframe is set while a
// project is loading, so the animation is kept, the project is not modified, and the node is logged.
TEST(OfxProjectLoad, AnimatedValueChangedAfterLoadIsLogged)
{
    if ( !pluginsAvailable() ) {
        return;
    }
    QTemporaryDir tmpDir;
    ASSERT_TRUE( tmpDir.isValid() );
    AppInstancePtr app = appPTR->getTopLevelInstance();
    ProjectPtr project = app->getProject();

    NodePtr generator = NatronTestPlugin::createNode(kNatronTestPluginGenerator);
    NodePtr deriving = NatronTestPlugin::createNode(kNatronTestPluginProjectLoad);
    ASSERT_TRUE(generator && deriving);
    const int source = inputNamed(deriving, kOfxImageEffectSimpleSourceClipName);
    ASSERT_GE(source, 0);
    knob<KnobBool>(deriving, "setDerived")->setValue(true);
    ASSERT_TRUE( deriving->connectInput(generator, source) );
    ASSERT_GT(knob<KnobDouble>(deriving, "derived")->getValue(), 7.);
    KnobDoublePtr derived = knob<KnobDouble>(deriving, "derived");
    derived->setValue(7.); // the static value, which the plug-in sets in the action
    derived->setValueAtTime(0., 7., ViewSpec::all(), 0);
    derived->setValueAtTime(100., 7., ViewSpec::all(), 0);
    derived.reset();
    const std::string derivingName = deriving->getScriptName();
    const U64 savedAge = deriving->getKnobsAge();
    const QString dir = tmpDir.path() + QLatin1Char('/');
    QString savedPath;
    ASSERT_TRUE( project->saveProject( dir, QString::fromUtf8("ProjectLoadAnimated.ntp"), &savedPath ) );
    generator.reset();
    deriving.reset();

    testing::internal::CaptureStdout();
    const bool loaded = project->loadProject( dir, QString::fromUtf8("ProjectLoadAnimated.ntp") );
    const std::string output = testing::internal::GetCapturedStdout();
    ASSERT_TRUE(loaded);
    deriving = project->getNodeByName(derivingName);
    ASSERT_TRUE( bool(deriving) );
    derived = knob<KnobDouble>(deriving, "derived");
    EXPECT_TRUE( derived->isAnimated(0) );
    EXPECT_EQ( 7., derived->getValueAtTime(50.) );
    EXPECT_TRUE( project->isSaveUpToDate() );
    EXPECT_EQ( std::string(), loggedLine(output, kChangedLog) ) << output;
    const std::string line = loggedLine(output, kAnimatedLog);
    EXPECT_NE( std::string::npos, line.find(derivingName) ) << output;
    EXPECT_EQ( savedAge, deriving->getKnobsAge() );
    derived.reset();
    deriving.reset();

    project->resetProject();
}

// A project whose PyPlug was updated at load is saved right after the load (Project::loadProject()), but not if the
// plug-ins changed saved values in the actions after the load: the project stays modified, and the user decides.
// The PyPlug makes a node that derives "derived" from its Source, through the GroupInput: the action changes it only
// when the PyPlug is connected.
TEST(OfxProjectLoad, PyPlugUpdateIsNotSavedWhenValuesChanged)
{
    if ( !pluginsAvailable() ) {
        return;
    }
    QTemporaryDir tmpDir;
    ASSERT_TRUE( tmpDir.isValid() );
    const QString dir = tmpDir.path() + QLatin1Char('/');
    const char* const pyPlugID = "net.natron.test.ProjectLoadPyPlug";
    const QString moduleName = QString::fromUtf8("NatronTestProjectLoadPyPlug");
    const std::string script = std::string("from NatronEngine import *\n"
                                           "def getPluginID():\n"
                                           "    return '") + pyPlugID + "'\n"
                               "def getLabel():\n"
                               "    return 'ProjectLoadPyPlug'\n"
                               "def getVersion():\n"
                               "    return 2\n"
                               "def getGrouping():\n"
                               "    return 'Other/Test'\n"
                               "def createInstance(app, group):\n"
                               "    inp = app.createNode('" + PLUGINID_NATRON_INPUT + "', -1, group)\n"
                               "    n = app.createNode('" kNatronTestPluginProjectLoad "', -1, group)\n"
                               "    n.getParam('setDerived').setValue(True)\n"
                               "    for i in range(n.getMaxInputCount()):\n"
                               "        if n.getInputLabel(i) == '" kOfxImageEffectSimpleSourceClipName "':\n"
                               "            n.connectInput(i, inp)\n"
                               "    out = app.createNode('" + PLUGINID_NATRON_OUTPUT + "', -1, group)\n"
                               "    out.connectInput(0, n)\n";
    ASSERT_TRUE( writeFile( dir + moduleName + QString::fromUtf8(".py"), QByteArray( script.c_str() ) ) );
    std::string err, output;
    ASSERT_TRUE( NATRON_NAMESPACE::NATRON_PYTHON_NAMESPACE::interpretPythonScript("import sys\nsys.path.insert(0, '" + tmpDir.path().toStdString() + "')\n", &err, &output) ) << err;
    Plugin* plugin = appPTR->registerPlugin( dir, QStringList() << QString::fromUtf8("Other") << QString::fromUtf8("Test"),
                                             QString::fromUtf8(pyPlugID), QString::fromUtf8("ProjectLoadPyPlug"), QString(), QStringList(),
                                             false, false, 0, false, 2, 0, false );
    ASSERT_TRUE(plugin);
    plugin->setPythonModule(dir + moduleName);
    // NodeCollectionSerialization::restoreFromSerialization() offers the PyPlug update only when the preference
    // "loadFromPyFile" is off (the reverse of its tooltip, since 2015)
    KnobBoolPtr loadFromPyFile = std::dynamic_pointer_cast<KnobBool>( appPTR->getCurrentSettings()->getKnobByName("loadFromPyFile") );
    ASSERT_TRUE( bool(loadFromPyFile) );
    const bool oldLoadFromPyFile = loadFromPyFile->getValue();
    loadFromPyFile->setValue(false);

    AppInstancePtr app = appPTR->getTopLevelInstance();
    ProjectPtr project = app->getProject();
    // two projects with the PyPlug, version 1: connected to a source (the action changes "derived") or not
    for (int connected = 0; connected < 2; ++connected) {
        const QString name = QString::fromUtf8("ProjectLoadPyPlug%1.ntp").arg(connected);
        project->resetProject();
        CreateNodeArgs args( pyPlugID, project );
        args.setProperty<bool>(kCreateNodeArgsPropAddUndoRedoCommand, false);
        args.setProperty<bool>(kCreateNodeArgsPropAutoConnect, false);
        NodePtr pyPlug = app->createNode(args);
        ASSERT_TRUE( bool(pyPlug) );
        ASSERT_EQ( 1, pyPlug->getNInputs() );
        if (connected) {
            NodePtr generator = NatronTestPlugin::createNode(kNatronTestPluginGenerator);
            ASSERT_TRUE( pyPlug->connectInput(generator, 0) );
        }
        pyPlug.reset();
        QString savedPath;
        ASSERT_TRUE( project->saveProject( dir, name, &savedPath ) );
        QByteArray contents = readFile(dir + name);
        const QByteArray version2("<PythonModuleVersion>2</PythonModuleVersion>");
        const QByteArray version1("<PythonModuleVersion>1</PythonModuleVersion>");
        ASSERT_EQ( 1, contents.count(version2) );
        contents.replace(version2, version1);
        ASSERT_TRUE( writeFile(dir + name, contents) );
    }
    project->resetProject();

    for (int connected = 0; connected < 2; ++connected) {
        const QString name = QString::fromUtf8("ProjectLoadPyPlug%1.ntp").arg(connected);
        testing::internal::CaptureStdout();
        const bool loaded = project->loadProject(dir, name);
        output = testing::internal::GetCapturedStdout();
        ASSERT_TRUE(loaded) << output;
        // the update was offered and accepted (the background app answers yes): see the unconnected case
        const QByteArray contents = readFile(dir + name);
        const std::string line = loggedLine(output, kChangedLog);
        if (connected) {
            // not saved: the file still has version 1, and the project is modified
            EXPECT_EQ( 1, contents.count("<PythonModuleVersion>1</PythonModuleVersion>") ) << "connected " << connected;
            EXPECT_FALSE( project->isSaveUpToDate() );
            EXPECT_NE( std::string::npos, line.find("The updated PyPlugs were not saved") ) << output;
        } else {
            // saved right after the load, with version 2
            EXPECT_EQ( 1, contents.count("<PythonModuleVersion>2</PythonModuleVersion>") ) << "connected " << connected;
            EXPECT_TRUE( project->isSaveUpToDate() );
            EXPECT_EQ( std::string(), line ) << output;
        }
    }

    project->resetProject();
    loadFromPyFile->setValue(oldLoadFromPyFile);
    ASSERT_TRUE( NATRON_NAMESPACE::NATRON_PYTHON_NAMESPACE::interpretPythonScript("import sys\nsys.path.remove('" + tmpDir.path().toStdString() + "')\n", &err, &output) ) << err;
}
