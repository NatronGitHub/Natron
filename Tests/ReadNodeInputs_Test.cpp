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

// The Read node has one input, which it hands to its decoder as input 0, but a reader doesn't have to
// define a Source clip (#1826). The test decoder of Tests/OfxTestPlugin/NatronTestPluginNoSourceReader.cpp
// only has an Output clip: creating a Read node with it, and connecting the Read node's input, must not
// read past the end of the decoder's clips.

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <iostream>
#include <list>
#include <string>

#include <QtCore/QTemporaryDir>

#include <gtest/gtest.h>

#include <ofxImageEffect.h>

#include "Engine/AppInstance.h"
#include "Engine/AppManager.h"
#include "Engine/CreateNodeArgs.h"
#include "Engine/EffectInstance.h"
#include "Engine/ImagePlaneDesc.h"
#include "Engine/Node.h"
#include "Engine/OfxEffectInstance.h"
#include "Engine/Plugin.h"
#include "Engine/Project.h"
#include "Engine/ReadNode.h"

#include "OfxTestPlugin.h"

NATRON_NAMESPACE_USING

namespace {
const int kNoSourceMajor = 90;

NodePtr
createProjectNode(const char* pluginID,
                  const std::string& filename = std::string())
{
    AppInstancePtr app = appPTR->getTopLevelInstance();
    CreateNodeArgs args( pluginID, app->getProject() );

    if ( !filename.empty() ) {
        args.addParamDefaultValue<std::string>(kOfxImageEffectFileParamName, filename);
    }
    args.setProperty<bool>(kCreateNodeArgsPropAddUndoRedoCommand, false);
    args.setProperty<bool>(kCreateNodeArgsPropAutoConnect, false);
    args.setProperty<bool>(kCreateNodeArgsPropSettingsOpened, false);

    return app->createNode(args);
}

// Destroys the nodes when the test ends, also when an ASSERT fails
struct DestroyNodesOnExit
{
    std::list<NodePtr> nodes;

    ~DestroyNodesOnExit()
    {
        for (std::list<NodePtr>::iterator it = nodes.begin(); it != nodes.end(); ++it) {
            if (*it) {
                (*it)->destroyNode(true /*blockingDestroy*/, false /*autoReconnect*/);
            }
        }
    }
};
} // anonymous namespace

TEST(ReadNodeInputs, DecoderWithoutSourceClip)
{
    Plugin* plugin = 0;
    try {
        plugin = appPTR->getPluginBinary(QString::fromUtf8(PLUGINID_OFX_READPDF), -1, -1, false);
    } catch (...) {
        plugin = 0;
    }
    if (!plugin) {
        NatronTestPlugin::reportMissing(PLUGINID_OFX_READPDF);

        return;
    }
    if (plugin->getMajorVersion() != kNoSourceMajor) {
        // a real ReadPDF with a major of 91 or more is installed
        NatronTest::skip("the newest " PLUGINID_OFX_READPDF " is version " + std::to_string( plugin->getMajorVersion() ) +
                         ", not the test reader");

        return;
    }
    QTemporaryDir tmpDir;
    ASSERT_TRUE( tmpDir.isValid() );
    const std::string filename = tmpDir.path().toStdString() + "/image.natrontestnosource";
    DestroyNodesOnExit destroyNodes;

    // creating the node computes the components accepted by the Read node's input
    NodePtr readNode = createProjectNode(PLUGINID_OFX_READPDF, filename);
    destroyNodes.nodes.push_back(readNode);
    ASSERT_TRUE( bool(readNode) );
    ReadNode* read = dynamic_cast<ReadNode*>( readNode->getEffectInstance().get() );
    ASSERT_TRUE(read);
    NodePtr decoder = read->getEmbeddedReader();
    ASSERT_TRUE( bool(decoder) );
    EXPECT_EQ( std::string(PLUGINID_OFX_READPDF), decoder->getPluginID() );
    OfxEffectInstance* ofxDecoder = dynamic_cast<OfxEffectInstance*>( decoder->getEffectInstance().get() );
    ASSERT_TRUE(ofxDecoder);
    EXPECT_EQ( 0, ofxDecoder->getNInputs() );
    EXPECT_EQ( 1, read->getNInputs() );

    // the decoder has no clip for an input
    EXPECT_EQ( nullptr, ofxDecoder->getClipCorrespondingToInput(0) );
    EXPECT_EQ( nullptr, ofxDecoder->getClipCorrespondingToInput(-1) );

    // the Read node's input still accepts images
    std::list<ImagePlaneDesc> comps;
    read->addAcceptedComponents(0, &comps);
    EXPECT_FALSE( comps.empty() );

    // connecting and disconnecting the input calls the Read node's onInputChanged()
    NodePtr dot = createProjectNode(PLUGINID_NATRON_DOT);
    destroyNodes.nodes.push_back(dot);
    ASSERT_TRUE( bool(dot) );
    EXPECT_TRUE( readNode->connectInput(dot, 0) );
    EXPECT_EQ( 0, readNode->disconnectInput(0) );
}
