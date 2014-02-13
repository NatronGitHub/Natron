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


#ifndef BASETEST_H
#define BASETEST_H

#include <vector>

#include "Global/Macros.h"

#include <gtest/gtest.h>

CLANG_DIAG_OFF(deprecated)
#include <QString>
CLANG_DIAG_ON(deprecated)

class AppInstance;

namespace Natron{
class Node;
}

class BaseTest : public testing::Test
{
public:
    BaseTest();

    virtual ~BaseTest();

protected:

    virtual void SetUp();

    virtual void TearDown();

    ///Useful function to create a node for all the tests.
    ///You should not call this function
    Natron::Node* createNode(const QString& pluginID,int majorVersion = -1,int minorVersion = -1);
    
    ///Useful function to connect 2 nodes together. It connects the input number inputNumber of
    ///output to the node input. expectedReturnValue is expected to have the same value as the return
    ///value of the underlying connect call. That means that if expectedReturnValue is true, the
    ///connection is expected to succeed, and vice versa.
    void connectNodes(Natron::Node* input,Natron::Node* output,int inputNumber,bool expectedReturnValue);
    
    ///Useful function to disconnect 2 nodes together. expectedReturnValue is expected to have the same value as the return
    ///value of the underlying disconnect call. That means that if expectedReturnValue is true, the
    ///disconnection is expected to succeed, and vice versa.
    void disconnectNodes(Natron::Node* input,Natron::Node* output,bool expectedReturnvalue);
    
    void registerTestPlugins();
    
    ///////////////Pointers to plug-ins that might be used by all the tests. This makes
    ///////////////it easy to create a node for a specific plug-in, you just have to call
    /////////////// createNode(<pluginID>).
    ///////////////If you add a test that need another plug-in, just add it to this list and in the
    ///////////////function registerTestPlugin
    ///////////////Adding a plugin here means that the user running the test MUST have the plugin
    ///////////////installed in order to the test to run, so don't add fancy plug-ins, you should
    ///////////////stick to the default plug-ins that are pre-compiled in the shell-script.
    QString _genericTestPluginID;
    QString _gainPluginID;
    QString _dotGeneratorPluginID;
    QString _readQtPluginID;
    QString _writeQtPluginID;
    
    std::vector<QString> _allTestPluginIDs;
    
    AppInstance* _app;
};

#endif // BASETEST_H
