/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://natrongithub.github.io/>,
 * Copyright (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
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

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <cstdlib>

#include "BaseTest.h"

#include <QtCore/QFile>
#include <QtCore/QThreadPool>

// ofxhPropertySuite.h:565:37: warning: 'this' pointer cannot be null in well-defined C++ code; comparison may be assumed to always evaluate to true [-Wtautological-undefined-compare]
CLANG_DIAG_OFF(unknown-pragmas)
CLANG_DIAG_OFF(tautological-undefined-compare) // appeared in clang 3.5
#include <ofxhPluginCache.h>
//#include <ofxhPluginAPICache.h>
//#include <ofxhImageEffect.h>
#include <ofxhImageEffectAPI.h>
//#include <ofxOpenGLRender.h>
//#include <ofxhHost.h>
CLANG_DIAG_ON(tautological-undefined-compare)
CLANG_DIAG_ON(unknown-pragmas)

#include "Engine/CreateNodeArgs.h"
#include "Engine/Node.h"
#include "Engine/Project.h"
#include "Engine/AppManager.h"
#include "Engine/AppInstance.h"
#include "Engine/KnobTypes.h"
#include "Engine/EffectInstance.h"
#include "Engine/Plugin.h"
#include "Engine/Curve.h"
#include "Engine/CLArgs.h"
#include "Engine/RenderQueue.h"
#include "Engine/Settings.h"
#include "Engine/ViewIdx.h"

NATRON_NAMESPACE_USING

BaseTest::BaseTest()
    : testing::Test()
    , _app()
{
}

BaseTest::~BaseTest()
{
}

void
BaseTest::registerTestPlugins()
{
    _allTestPluginIDs.clear();

    _generatorPluginID = QString::fromUtf8(PLUGINID_OFX_SENOISE);
    _allTestPluginIDs.push_back(_generatorPluginID);

    _readOIIOPluginID = QString::fromUtf8(PLUGINID_OFX_READOIIO);
    _allTestPluginIDs.push_back(_readOIIOPluginID);

    _writeOIIOPluginID = QString::fromUtf8(PLUGINID_OFX_WRITEOIIO);
    _allTestPluginIDs.push_back(_writeOIIOPluginID);

    for (unsigned int i = 0; i < _allTestPluginIDs.size(); ++i) {
        ///make sure the generic test plugin is present
        PluginPtr p;
        try {
            p = appPTR->getPluginBinary(_allTestPluginIDs[i], -1, -1, false);
        } catch (const std::exception & e) {
            std::cout << e.what() << std::endl;
        }

        ASSERT_TRUE(p != NULL);
    }
}

void
BaseTest::SetUp()
{
    const char* path = std::getenv("OFX_PLUGIN_PATH");
    if (path) {
        std::cout << "Warning: Ignoring standard plugin path, OFX_PLUGIN_PATH=" << path << std::endl;
        OFX::Host::PluginCache::useStdOFXPluginsLocation(false);
    }

    _app = appPTR->getTopLevelInstance();
    registerTestPlugins();
}

void
BaseTest::TearDown()
{
    appPTR->getCurrentSettings()->setNumberOfThreads(0);
    ///Caches may have launched some threads to delete images, wait for them to be done
    QThreadPool::globalInstance()->waitForDone();
}

NodePtr
BaseTest::createNode(const QString & pluginID,
                     int majorVersion,
                     int minorVersion)
{
    CreateNodeArgsPtr args(CreateNodeArgs::create( pluginID.toStdString(), getApp()->getProject() ));
    
    args->setProperty<int>(kCreateNodeArgsPropPluginVersion, majorVersion, 0);
    args->setProperty<int>(kCreateNodeArgsPropPluginVersion, minorVersion, 1);

    NodePtr ret =  getApp()->createNode(args);


    EXPECT_NE(ret.get(), (Node*)NULL);

    return ret;
}

void
BaseTest::connectNodes(const NodePtr& input,
                       const NodePtr& output,
                       int inputNumber,
                       bool expectedReturnValue)
{
    if (expectedReturnValue) {
        ///check that the connections are internally all set as "expected"

        EXPECT_EQ( (Node*)NULL, output->getInput(inputNumber).get() );
        EXPECT_FALSE( output->isInputConnected(inputNumber) );
    } else {
        ///the call can only fail for those 2 reasons
        EXPECT_TRUE(inputNumber > output->getNInputs() || //< inputNumber is greater than the maximum input number
                    output->getInput(inputNumber).get() != (Node*)NULL); //< input slot is already filled with another node
    }


    bool ret = output->connectInput(input, inputNumber);
    EXPECT_EQ(expectedReturnValue, ret);

    if (expectedReturnValue) {
        EXPECT_TRUE( input->hasOutputConnected() );
        EXPECT_EQ(output->getInput(inputNumber), input);
        EXPECT_TRUE( output->isInputConnected(inputNumber) );
    }
}

void
BaseTest::disconnectNodes(const NodePtr& input,
                          const NodePtr& output,
                          bool expectedReturnvalue)
{
    if (expectedReturnvalue) {
        ///check that the connections are internally all set as "expected"

        ///the input must have in its output the node 'output'
        EXPECT_TRUE( input->hasOutputConnected() );
        OutputNodesMap outputs;
        input->getOutputs(outputs);
        OutputNodesMap::const_iterator foundOutput = outputs.find(output);

        ///the output must have in its inputs the node 'input'
        std::list<int> connectedInputs = input->getInputIndicesConnectedToThisNode(output);

        EXPECT_TRUE(!connectedInputs.empty());
        EXPECT_TRUE(foundOutput != outputs.end());
        for (std::list<int>::const_iterator it = connectedInputs.begin(); it!=connectedInputs.end(); ++it) {
            EXPECT_EQ(output->getInput(*it), input);
            EXPECT_TRUE( output->isInputConnected(*it) );
        }
    }

    ///call disconnect
    bool ret = output->disconnectInput(input);
    EXPECT_EQ(expectedReturnvalue, ret);

    if (expectedReturnvalue) {
        ///check that the disconnection went OK

        OutputNodesMap outputs;
        input->getOutputs(outputs);
        OutputNodesMap::const_iterator foundOutput = outputs.find(output);


        ///the output must have in its inputs the node 'input'
        std::list<int> connectedInputs = input->getInputIndicesConnectedToThisNode(output);

        EXPECT_TRUE(foundOutput == outputs.end());
        EXPECT_TRUE(connectedInputs.empty());
        for (std::list<int>::const_iterator it = connectedInputs.begin(); it!=connectedInputs.end(); ++it) {
            EXPECT_EQ( (Node*)NULL, output->getInput(*it).get() );
            EXPECT_FALSE( output->isInputConnected(*it) );
        }
    }
} // disconnectNodes

///High level test: render 1 frame of dot generator
TEST_F(BaseTest, GenerateDot)
{
    ///create the generator
    NodePtr generator = createNode(_generatorPluginID);

    ///create the writer and set its output filename
    NodePtr writer = createNode(_writeOIIOPluginID);

    ASSERT_TRUE( bool(generator) && bool(writer) );

    KnobIPtr frameRange = generator->getApp()->getProject()->getKnobByName("frameRange");
    ASSERT_TRUE( bool(frameRange) );
    KnobIntPtr knob = toKnobInt(frameRange);
    ASSERT_TRUE( bool(knob) );
    knob->setValue(1, ViewSetSpec::all(), DimIdx(0));
    knob->setValue(1, ViewSetSpec::all(), DimIdx(1));

    Format f(0, 0, 200, 200, "toto", 1.);
    generator->getApp()->getProject()->setOrAddProjectFormat(f);

    std::string binPath = appPTR->getApplicationBinaryDirPath();
    std::string filePath = binPath + std::string("/test_dot_generator.jpg");
    writer->getEffectInstance()->setOutputFilesForWriter( filePath);

    ///attempt to connect the 2 nodes together
    connectNodes(generator, writer, 0, true);

    ///and start rendering. This call is blocking.
    std::list<RenderQueue::RenderWork> works;
    RenderQueue::RenderWork w;
    w.treeRoot = writer;
    works.push_back(w);
    getApp()->getRenderQueue()->renderBlocking(works);

    EXPECT_TRUE( QFile::exists(QString::fromUtf8(filePath.c_str())) );
    QFile::remove(QString::fromUtf8(filePath.c_str()));
}

TEST_F(BaseTest, SetValues)
{
    NodePtr generator = createNode(_generatorPluginID);

    assert(generator);
    KnobDoublePtr knob = toKnobDouble(generator->getKnobByName("noiseZ"));
    EXPECT_TRUE( bool(knob) );
    if (!knob) {
        return;
    }
    knob->setValue(0.5);
    EXPECT_TRUE(knob->getValue() == 0.5);

    //Check that linear interpolation is working as intended
    KeyFrame kf;
    knob->setInterpolationAtTime(ViewSetSpec::all(),  DimIdx(0),  TimeValue(0), eKeyframeTypeLinear, &kf);
    knob->setValueAtTime(TimeValue(0), 0., ViewSetSpec::all(), DimIdx(0));
    knob->setValueAtTime(TimeValue(100), 1., ViewSetSpec::all(), DimIdx(0));
    for (int i = 0; i <= 100; ++i) {
        double v = knob->getValueAtTime(TimeValue(i));
        EXPECT_TRUE(std::abs(v - i / 100.) < 1e-6);
    }
}

///High level test: simple node connections test
TEST_F(BaseTest, SimpleNodeConnections) {
    ///create the generator
    NodePtr generator = createNode(_generatorPluginID);

    ///create the writer and set its output filename
    NodePtr writer = createNode(_writeOIIOPluginID);

    ASSERT_TRUE(writer && generator);
    connectNodes(generator, writer, 0, true);
    connectNodes(generator, writer, 0, false); //< expect it to fail
    disconnectNodes(generator, writer, true);
    disconnectNodes(generator, writer, false);
    connectNodes(generator, writer, 0, true);
}
