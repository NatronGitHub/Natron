//  Natron
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 * Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 * contact: immarespond at gmail dot com
 *
 */


#include "BaseTest.h"

#include "Engine/Node.h"
#include "Engine/Project.h"
#include "Engine/AppManager.h"
#include "Engine/AppInstance.h"

using namespace Natron;


BaseTest::BaseTest()
    : testing::Test()
      , _genericTestPluginID()
      , _gainPluginID()
      , _dotGeneratorPluginID()
      , _readOIIOPluginID()
      , _writeOIIOPluginID()
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

    _genericTestPluginID = QString("GenericTest  [OFX]");
    _allTestPluginIDs.push_back(_genericTestPluginID);

    _gainPluginID =  QString("Gain  [OFX]");
    _allTestPluginIDs.push_back(_gainPluginID);

    _dotGeneratorPluginID = QString("Dot Generator  [OFX]");
    _allTestPluginIDs.push_back(_dotGeneratorPluginID);

    _readOIIOPluginID = QString("ReadOIIOOFX  [Image]");
    _allTestPluginIDs.push_back(_readOIIOPluginID);

    _writeOIIOPluginID = QString("WriteOIIOOFX  [Image]");
    _allTestPluginIDs.push_back(_writeOIIOPluginID);

    for (unsigned int i = 0; i < _allTestPluginIDs.size(); ++i) {
        ///make sure the generic test plugin is present
        Natron::LibraryBinary* bin = NULL;
        try {
            bin = appPTR->getPluginBinary(_allTestPluginIDs[i], -1, -1);
        } catch (const std::exception & e) {
            std::cout << e.what() << std::endl;
        }

        ASSERT_TRUE(bin != NULL);
    }
}

void
BaseTest::SetUp()
{
    AppManager* manager = new AppManager;
    int argc = 0;

    manager->load(argc,NULL);
    //////WARNING: This test disables multi-threading! if it fails it will never re-enable it
    ////// hence the next time you launch the application multi-threading will be DISABLED.
    manager->setNumberOfThreads(-1);
    _app = manager->getTopLevelInstance();

    registerTestPlugins();
}

void
BaseTest::TearDown()
{
    _app->quit();
    _app = 0;
    appPTR->setNumberOfThreads(0);
    delete appPTR;
}

boost::shared_ptr<Natron::Node> BaseTest::createNode(const QString & pluginID,
                                                     int majorVersion,
                                                     int minorVersion)
{
    boost::shared_ptr<Node> ret =  _app->createNode( CreateNodeArgs(pluginID,
                                                                    "",
                                                                    majorVersion,minorVersion,-1,true,INT_MIN,INT_MIN,true,true,
                                                                    QString(),CreateNodeArgs::DefaultValuesList()) );

    EXPECT_NE(ret.get(),(Natron::Node*)NULL);

    return ret;
}

void
BaseTest::connectNodes(boost::shared_ptr<Natron::Node> input,
                       boost::shared_ptr<Natron::Node> output,
                       int inputNumber,
                       bool expectedReturnValue)
{
    if (expectedReturnValue) {
        ///check that the connections are internally all set as "expected"

        EXPECT_EQ( (Natron::Node*)NULL,output->getInput(inputNumber).get() );
        EXPECT_FALSE( output->isInputConnected(inputNumber) );
    } else {
        ///the call can only fail for those 2 reasons
        EXPECT_TRUE(inputNumber > output->getMaxInputCount() || //< inputNumber is greater than the maximum input number
                    output->getInput(inputNumber).get() != (Natron::Node*)NULL); //< input slot is already filled with another node
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
BaseTest::disconnectNodes(boost::shared_ptr<Natron::Node> input,
                          boost::shared_ptr<Natron::Node> output,
                          bool expectedReturnvalue)
{
    if (expectedReturnvalue) {
        ///check that the connections are internally all set as "expected"

        ///the input must have in its output the node 'output'
        EXPECT_TRUE( input->hasOutputConnected() );
        const std::list<boost::shared_ptr<Natron::Node> > & outputs = input->getOutputs();
        bool foundOutput = false;
        for (std::list<boost::shared_ptr<Natron::Node> >::const_iterator it = outputs.begin(); it != outputs.end(); ++it) {
            if ( (*it) == output ) {
                foundOutput = true;
                break;
            }
        }

        ///the output must have in its inputs the node 'input'
        const std::vector<boost::shared_ptr<Natron::Node> > & inputs = output->getInputs_mt_safe();
        int inputIndex = 0;
        bool foundInput = false;
        for (U32 i = 0; i < inputs.size(); ++i) {
            if (inputs[i] == input) {
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

        const std::list<boost::shared_ptr<Natron::Node> > & outputs = input->getOutputs();
        bool foundOutput = false;
        for (std::list<boost::shared_ptr<Natron::Node> >::const_iterator it = outputs.begin(); it != outputs.end(); ++it) {
            if ( (*it) == output ) {
                foundOutput = true;
                break;
            }
        }

        ///the output must have in its inputs the node 'input'
        const std::vector<boost::shared_ptr<Natron::Node> > & inputs = output->getInputs_mt_safe();
        int inputIndex = 0;
        bool foundInput = false;
        for (U32 i = 0; i < inputs.size(); ++i) {
            if (inputs[i] == input) {
                foundInput = true;
                break;
            }
            ++inputIndex;
        }

        EXPECT_FALSE(foundOutput);
        EXPECT_FALSE(foundInput);
        EXPECT_EQ( (Natron::Node*)NULL,output->getInput(inputIndex).get() );
        EXPECT_FALSE( output->isInputConnected(inputIndex) );
    }
} // disconnectNodes

///High level test: render 1 frame of dot generator
TEST_F(BaseTest,GenerateDot)
{
    ///create the generator
    boost::shared_ptr<Node> generator = createNode(_dotGeneratorPluginID);

    ///create the writer and set its output filename
    boost::shared_ptr<Node> writer = createNode(_writeOIIOPluginID);

    writer->setOutputFilesForWriter("test_dot_generator#.jpg");

    ///attempt to connect the 2 nodes together
    connectNodes(generator, writer, 0, true);

    ///and start rendering. This call is blocking.
    std::list<AppInstance::RenderWork> works;
    AppInstance::RenderWork w;
    w.writer = dynamic_cast<Natron::OutputEffectInstance*>(writer->getLiveInstance());
    assert(w.writer);
    w.firstFrame = INT_MIN;
    w.lastFrame = INT_MAX;
    works.push_back(w);
    _app->startWritersRendering(works);
}

///High level test: simple node connections test
TEST_F(BaseTest,SimpleNodeConnections) {
    ///create the generator
    boost::shared_ptr<Node> generator = createNode(_dotGeneratorPluginID);

    ///create the writer and set its output filename
    boost::shared_ptr<Node> writer = createNode(_writeOIIOPluginID);

    connectNodes(generator, writer, 0, true);
    connectNodes(generator, writer, 0, false); //< expect it to fail
    disconnectNodes(generator, writer, true);
    disconnectNodes(generator, writer, false);
    connectNodes(generator, writer, 0, true);
}
