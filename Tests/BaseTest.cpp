/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2016 INRIA and Alexandre Gauthier-Foichat
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


#include "BaseTest.h"

#include <QFile>

#include "Engine/Node.h"
#include "Engine/Project.h"
#include "Engine/AppManager.h"
#include "Engine/AppInstance.h"
#include "Engine/KnobTypes.h"
#include "Engine/EffectInstance.h"
#include "Engine/Plugin.h"
#include "Engine/Curve.h"
#include "Engine/CLArgs.h"
#include "Engine/ViewIdx.h"

NATRON_NAMESPACE_USING

static AppManager* g_manager = 0;

BaseTest::BaseTest()
    : testing::Test()
      , _app(0)
{
}

BaseTest::~BaseTest()
{
}

void
BaseTest::registerTestPlugins()
{
    _allTestPluginIDs.clear();

    _dotGeneratorPluginID = PLUGINID_OFX_DOTEXAMPLE;
    _allTestPluginIDs.push_back(_dotGeneratorPluginID);

    _readOIIOPluginID = PLUGINID_OFX_READOIIO;
    _allTestPluginIDs.push_back(_readOIIOPluginID);

    _writeOIIOPluginID = PLUGINID_OFX_WRITEOIIO;
    _allTestPluginIDs.push_back(_writeOIIOPluginID);

    for (unsigned int i = 0; i < _allTestPluginIDs.size(); ++i) {
        ///make sure the generic test plugin is present
        LibraryBinary* bin = NULL;
        try {
            Plugin* p = appPTR->getPluginBinary(_allTestPluginIDs[i], -1, -1, false);
            if (p) {
                bin = p->getLibraryBinary();
            }
            
        } catch (const std::exception & e) {
            std::cout << e.what() << std::endl;
        }

        ASSERT_TRUE(bin != NULL);
    }
}

void
BaseTest::SetUp()
{
    if (!g_manager) {
        g_manager = new AppManager;
        int argc = 0;
        CLArgs cl;
        g_manager->load(argc, 0, cl);
    }

    _app = g_manager->getTopLevelInstance();
    registerTestPlugins();

}

void
BaseTest::TearDown()
{
    appPTR->setNumberOfThreads(0);
 
}

NodePtr BaseTest::createNode(const QString & pluginID,
                                                     int majorVersion,
                                                     int minorVersion)
{
    CreateNodeArgs args(pluginID, eCreateNodeReasonInternal, _app->getProject());
    args.majorV = majorVersion;
    args.minorV = minorVersion;
    NodePtr ret =  _app->createNode(args);
                                                    

    EXPECT_NE(ret.get(),(Node*)NULL);

    return ret;
}

void
BaseTest::connectNodes(NodePtr input,
                       NodePtr output,
                       int inputNumber,
                       bool expectedReturnValue)
{
    if (expectedReturnValue) {
        ///check that the connections are internally all set as "expected"

        EXPECT_EQ( (Node*)NULL,output->getInput(inputNumber).get() );
        EXPECT_FALSE( output->isInputConnected(inputNumber) );
    } else {
        ///the call can only fail for those 2 reasons
        EXPECT_TRUE(inputNumber > output->getMaxInputCount() || //< inputNumber is greater than the maximum input number
                    output->getInput(inputNumber).get() != (Node*)NULL); //< input slot is already filled with another node
    }


    bool ret = _app->getProject()->connectNodes(inputNumber,input,output);
    EXPECT_EQ(expectedReturnValue,ret);

    if (expectedReturnValue) {
        EXPECT_TRUE( input->hasOutputConnected() );
        EXPECT_EQ(output->getInput(inputNumber),input);
        EXPECT_TRUE( output->isInputConnected(inputNumber) );
    }
}

void
BaseTest::disconnectNodes(NodePtr input,
                          NodePtr output,
                          bool expectedReturnvalue)
{
    if (expectedReturnvalue) {
        ///check that the connections are internally all set as "expected"

        ///the input must have in its output the node 'output'
        EXPECT_TRUE( input->hasOutputConnected() );
        const NodesWList & outputs = input->getGuiOutputs();
        bool foundOutput = false;
        for (NodesWList::const_iterator it = outputs.begin(); it != outputs.end(); ++it) {
            if ( it->lock() == output) {
                foundOutput = true;
                break;
            }
        }

        ///the output must have in its inputs the node 'input'
        const std::vector<NodeWPtr> & inputs = output->getGuiInputs();
        int inputIndex = 0;
        bool foundInput = false;
        for (U32 i = 0; i < inputs.size(); ++i) {
            if (inputs[i].lock() == input) {
                foundInput = true;
                break;
            }
            ++inputIndex;
        }

        EXPECT_TRUE(foundInput);
        EXPECT_TRUE(foundOutput);
        EXPECT_EQ(output->getInput(inputIndex),input);
        EXPECT_TRUE( output->isInputConnected(inputIndex) );
    }

    ///call disconnect
    bool ret = _app->getProject()->disconnectNodes(input,output);
    EXPECT_EQ(expectedReturnvalue,ret);

    if (expectedReturnvalue) {
        ///check that the disconnection went OK

        const NodesWList & outputs = input->getGuiOutputs();
        bool foundOutput = false;
        for (NodesWList::const_iterator it = outputs.begin(); it != outputs.end(); ++it) {
            if (it->lock() == output) {
                foundOutput = true;
                break;
            }
        }

        ///the output must have in its inputs the node 'input'
        const std::vector<NodeWPtr> & inputs = output->getGuiInputs();
        int inputIndex = 0;
        bool foundInput = false;
        for (U32 i = 0; i < inputs.size(); ++i) {
            if (inputs[i].lock() == input) {
                foundInput = true;
                break;
            }
            ++inputIndex;
        }

        EXPECT_FALSE(foundOutput);
        EXPECT_FALSE(foundInput);
        EXPECT_EQ( (Node*)NULL,output->getInput(inputIndex).get() );
        EXPECT_FALSE( output->isInputConnected(inputIndex) );
    }
} // disconnectNodes

///High level test: render 1 frame of dot generator
TEST_F(BaseTest,GenerateDot)
{
    ///create the generator
    NodePtr generator = createNode(_dotGeneratorPluginID);
    
    ///create the writer and set its output filename
    NodePtr writer = createNode(_writeOIIOPluginID);
    ASSERT_TRUE(generator && writer);
    
    KnobPtr frameRange = generator->getApp()->getProject()->getKnobByName("frameRange");
    ASSERT_TRUE(frameRange);
    KnobInt* knob = dynamic_cast<KnobInt*>(frameRange.get());
    ASSERT_TRUE(knob);
    knob->setValue(1, ViewSpec::all(), 0);
    knob->setValue(1, ViewSpec::all(), 1);
    
    const QString& binPath = appPTR->getApplicationBinaryPath();
    QString filePath = binPath + "/test_dot_generator.jpg";
    writer->setOutputFilesForWriter(filePath.toStdString());

    ///attempt to connect the 2 nodes together
    connectNodes(generator, writer, 0, true);

    ///and start rendering. This call is blocking.
    std::list<AppInstance::RenderWork> works;
    AppInstance::RenderWork w;
    w.writer = dynamic_cast<OutputEffectInstance*>(writer->getEffectInstance().get());
    assert(w.writer);
    w.firstFrame = INT_MIN;
    w.lastFrame = INT_MAX;
    w.frameStep = INT_MIN;
    works.push_back(w);
    _app->startWritersRendering(false, false, works);
    
    EXPECT_TRUE(QFile::exists(filePath));
    QFile::remove(filePath);
}

TEST_F(BaseTest,SetValues)
{
    NodePtr generator = createNode(_dotGeneratorPluginID);
    assert(generator);
    KnobPtr knob = generator->getKnobByName("radius");
    KnobDouble* radius = dynamic_cast<KnobDouble*>(knob.get());
    assert(radius);
    radius->setValue(100);
    EXPECT_TRUE(radius->getValue() == 100);
    
    //Check that linear interpolation is working as intended
    KeyFrame kf;
    radius->setInterpolationAtTime(eCurveChangeReasonInternal, ViewSpec::all(),  0, 0, eKeyframeTypeLinear, &kf);
    radius->setValueAtTime(0, ViewSpec::all(),  0, 0);
    radius->setValueAtTime(100, ViewSpec::all(),  100, 0);
    for (int i = 0; i <= 100; ++i) {
        double v = radius->getValueAtTime(i) ;
        EXPECT_TRUE(std::abs(v - i) < 1e-6);
    }
    
}

///High level test: simple node connections test
TEST_F(BaseTest,SimpleNodeConnections) {
    ///create the generator
    NodePtr generator = createNode(_dotGeneratorPluginID);

    ///create the writer and set its output filename
    NodePtr writer = createNode(_writeOIIOPluginID);
    ASSERT_TRUE(writer && generator);
    connectNodes(generator, writer, 0, true);
    connectNodes(generator, writer, 0, false); //< expect it to fail
    disconnectNodes(generator, writer, true);
    disconnectNodes(generator, writer, false);
    connectNodes(generator, writer, 0, true);
}
