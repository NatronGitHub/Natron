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
    AppManager* manager = AppManager::instance();
    manager->load(NULL);
    _app = manager->newAppInstance(AppInstance::APP_BACKGROUND);
    
    registerTestPlugins();

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

void BaseTest::SetUp() {
}

void BaseTest::TearDown() {
    delete _app;
    _app = 0;
    AppManager::quit();
}

Natron::Node* BaseTest::createNode(const QString& pluginID,int majorVersion,int minorVersion) {
    Node* ret =  _app->createNode(pluginID,majorVersion,minorVersion,true,false);
    EXPECT_TRUE(ret != (Node*)NULL);
    return ret;
}

TEST_F(BaseTest,GenerateDot) {
    Node* generator = createNode(_dotGeneratorPluginID);
    Node* writer = createNode(_writeQtPluginID);
    writer->setOutputFilesForWriter("test_dot_generator#.jpg");
    
    bool ret = _app->connect(0, generator, writer);
    ASSERT_TRUE(ret) << "Couldn't connect reader and writer";

    _app->startWritersRendering(QStringList(writer->getName().c_str()));
}
