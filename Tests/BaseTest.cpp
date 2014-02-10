//  Natron
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 *Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 *contact: immarespond at gmail dot com
 *
 */


#include "BaseTest.h"

#include "Engine/Node.h"
#include "Engine/Project.h"
#include "Global/AppManager.h"

using namespace Natron;

BaseTest::BaseTest()
    : testing::Test()
    , _genericTestPluginID()
    , _gainPluginID()
    , _dotGeneratorPluginID()
    , _readQtPluginID()
    , _writeQtPluginID()
    , _app(0)
{
}

BaseTest::~BaseTest() {
}

void BaseTest::registerTestPlugins() {

    _allTestPluginIDs.clear();

    _genericTestPluginID = QString("GenericTest  [OFX]");
    _allTestPluginIDs.push_back(_genericTestPluginID);
    
    _gainPluginID =  QString("Gain  [OFX]");
    _allTestPluginIDs.push_back(_gainPluginID);
    
    _dotGeneratorPluginID = QString("Dot Generator  [OFX]");
    _allTestPluginIDs.push_back(_dotGeneratorPluginID);

    _readQtPluginID = QString("ReadQt");
    _allTestPluginIDs.push_back(_readQtPluginID);

    _writeQtPluginID = QString("WriteQt");
    _allTestPluginIDs.push_back(_writeQtPluginID);
    
    for (unsigned int i = 0; i < _allTestPluginIDs.size(); ++i) {
        ///make sure the generic test plugin is present
        Natron::LibraryBinary* bin = NULL;
        try {
            bin = appPTR->getPluginBinary(_allTestPluginIDs[i], -1, -1);
        } catch (const std::exception& e) {
            std::cout << e.what() << std::endl;
        }
        ASSERT_TRUE(bin != NULL);
    }
}

void BaseTest::SetUp()
{
    AppManager* manager = AppManager::instance();
    manager->load(NULL);
    manager->setMultiThreadEnabled(false);
    _app = manager->newAppInstance(AppInstance::APP_BACKGROUND);
    
    registerTestPlugins();
}

void BaseTest::TearDown()
{
    delete _app;
    _app = 0;
    AppManager::quit();
}

Natron::Node* BaseTest::createNode(const QString& pluginID,int majorVersion,int minorVersion) {
    Node* ret =  _app->createNode(pluginID,majorVersion,minorVersion,true,false);
    EXPECT_TRUE(ret != (Node*)NULL);
    return ret;
}

void BaseTest::connectNodes(Natron::Node* input,Natron::Node* output,int inputNumber,bool expectedReturnValue) {
    
    if (expectedReturnValue) {
        ///check that the connections are internally all set as "expected"
        
        EXPECT_TRUE(output->input(inputNumber) == NULL);
        EXPECT_FALSE(output->isInputConnected(inputNumber));
    } else {
        
        ///the call can only fail for those 2 reasons
        EXPECT_TRUE(inputNumber > output->maximumInputs() || //< inputNumber is greater than the maximum input number
                    output->input(inputNumber) != NULL); //< input slot is already filled with another node
    }
    
    
    bool ret = _app->getProject()->connectNodes(inputNumber,input,output);
    EXPECT_TRUE(expectedReturnValue == ret);
    
    if (expectedReturnValue) {
        EXPECT_TRUE(input->hasOutputConnected());
        EXPECT_TRUE(output->input(inputNumber) == input);
        EXPECT_TRUE(output->isInputConnected(inputNumber));
    }
}

void BaseTest::disconnectNodes(Natron::Node* input,Natron::Node* output,bool expectedReturnvalue) {
    
    if (expectedReturnvalue) {
        ///check that the connections are internally all set as "expected"
        
        ///the input must have in its output the node 'output'
        EXPECT_TRUE(input->hasOutputConnected());
        const Natron::Node::OutputMap& outputs = input->getOutputs();
        bool foundOutput = false;
        for (Natron::Node::OutputMap::const_iterator it = outputs.begin(); it!=outputs.end(); ++it) {
            if (it->second == output) {
                foundOutput = true;
                break;
            }
        }
        
        ///the output must have in its inputs the node 'input'
        const Natron::Node::InputMap& inputs = output->getInputs();
        int inputIndex = 0;
        bool foundInput = false;
        for(Natron::Node::InputMap::const_iterator it = inputs.begin(); it!=inputs.end();++it) {
            if (it->second == input) {
                foundInput = true;
                break;
            }
            ++inputIndex;
        }
        
        EXPECT_TRUE(foundInput && foundOutput);
        EXPECT_TRUE(output->input(inputIndex) == input);
        EXPECT_TRUE(output->isInputConnected(inputIndex));
    }
    
    ///call disconnect
    bool ret = _app->getProject()->disconnectNodes(input,output);
    EXPECT_TRUE(expectedReturnvalue == ret);
    
    if (expectedReturnvalue) {
        ///check that the disconnection went OK
        
        const Natron::Node::OutputMap& outputs = input->getOutputs();
        bool foundOutput = false;
        for (Natron::Node::OutputMap::const_iterator it = outputs.begin(); it!=outputs.end();++it) {
            if (it->second == output) {
                foundOutput = true;
                break;
            }
        }
        
        ///the output must have in its inputs the node 'input'
        const Natron::Node::InputMap& inputs = output->getInputs();
        int inputIndex = 0;
        bool foundInput = false;
        for(Natron::Node::InputMap::const_iterator it = inputs.begin(); it!=inputs.end();++it) {
            if (it->second == input) {
                foundInput = true;
                break;
            }
            ++inputIndex;
        }
        
        EXPECT_TRUE(!foundOutput && !foundInput);
        EXPECT_TRUE(output->input(inputIndex) == NULL);
        EXPECT_FALSE(output->isInputConnected(inputIndex));
    }
}

///High level test: render 1 frame of dot generator
TEST_F(BaseTest,GenerateDot) {
    
    ///create the generator
    Node* generator = createNode(_dotGeneratorPluginID);
    
    ///create the writer and set its output filename
    Node* writer = createNode(_writeQtPluginID);
    writer->setOutputFilesForWriter("test_dot_generator#.jpg");
    
    ///attempt to connect the 2 nodes together
    connectNodes(generator, writer, 0, true);
    
    ///and start rendering. This call is blocking.
    _app->startWritersRendering(QStringList(writer->getName().c_str()));
}

///High level test: simple node connections test
TEST_F(BaseTest,SimpleNodeConnections) {
    ///create the generator
    Node* generator = createNode(_dotGeneratorPluginID);
    
    ///create the writer and set its output filename
    Node* writer = createNode(_writeQtPluginID);
    
    connectNodes(generator, writer, 0, true);
    connectNodes(generator, writer, 0, false); //< expect it to fail
    disconnectNodes(generator, writer, true);
    disconnectNodes(generator, writer, false);
    connectNodes(generator, writer, 0, true);
}
