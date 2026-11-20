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

// OFX 1.5 (ofxCore.h, kOfxActionInstanceChanged): "when a host loads a plugin instance from a saved project, if the
// input clips are not connected at that time, the host must call `instanceChanged` after kOfxActionCreateInstance and
// after the host has hooked up all clips, with property kOfxPropType = kOfxTypeClip and kOfxPropChangeReason =
// kOfxChangePluginEdited, and ensure that OfxImageEffectSuiteV1::clipGetRegionOfDefinition works in this action"
// (#1107). The test plug-in net.natron.test.ProjectLoad (Tests/OfxTestPlugin/NatronTestPluginProjectLoad.cpp) counts
// these actions and, like a writer, sets parameters from its source clip when the user connects it: the values saved
// in the project must be the ones it has after the project is loaded.

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
#include "Engine/KnobTypes.h"
#include "Engine/Node.h"
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
    "ProjectLoad.connected", "ProjectLoad.rod",
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
// kOfxChangePluginEdited, in a bracket, with a working clipGetRegionOfDefinition; a plug-in without connected inputs
// gets none; and every value is the one that was saved.
TEST(OfxProjectLoad, InstanceChangedAfterLoadKeepsSavedValues)
{
    if ( !NatronTestPlugin::isAvailable(kNatronTestPluginProjectLoad) ||
         !NatronTestPlugin::isAvailable(kNatronTestPluginGenerator) ) {
        NatronTestPlugin::reportSkipped(kNatronTestPluginProjectLoad);

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
    const std::string connectedName = connected->getScriptName();
    const std::string aloneName = alone->getScriptName();
    const QString dir = tmpDir.path() + QLatin1Char('/');
    QString savedPath;
    ASSERT_TRUE( project->saveProject( dir, QString::fromUtf8("ProjectLoad.ntp"), &savedPath ) );
    generator.reset();
    connected.reset();
    alone.reset();

    start = readCounters();
    ASSERT_TRUE( project->loadProject( dir, QString::fromUtf8("ProjectLoad.ntp") ) );
    delta = countersSince(start);

    std::map<std::string, int> expected;
    expected["ProjectLoad.created"] = 2;
    expected["ProjectLoad.begin." kOfxChangePluginEdited] = 1;
    expected["ProjectLoad.end." kOfxChangePluginEdited] = 1;
    expected["ProjectLoad.clip." kOfxImageEffectSimpleSourceClipName "." kOfxChangePluginEdited] = 1;
    expected["ProjectLoad.connected"] = 1;
    expected["ProjectLoad.rod"] = 1;
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
    connected.reset();
    alone.reset();

    // leave the shared app with an untitled empty project, before the directory is removed
    project->resetProject();
}
