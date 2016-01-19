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


#ifndef BASETEST_H
#define BASETEST_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include <vector>

#include "Global/Macros.h"
#include <boost/shared_ptr.hpp>
#include <gtest/gtest.h>

CLANG_DIAG_OFF(deprecated)
#include <QString>
CLANG_DIAG_ON(deprecated)

#include "Engine/EngineFwd.h"

NATRON_NAMESPACE_ENTER

class BaseTest
    : public testing::Test
{
public:
    BaseTest();

    virtual ~BaseTest();

protected:

    virtual void SetUp();
    virtual void TearDown();

    ///Useful function to create a node for all the tests.
    ///You should not call this function
    boost::shared_ptr<Natron::Node> createNode(const QString & pluginID,int majorVersion = -1,int minorVersion = -1);

    ///Useful function to connect 2 nodes together. It connects the input number inputNumber of
    ///output to the node input. expectedReturnValue is expected to have the same value as the return
    ///value of the underlying connect call. That means that if expectedReturnValue is true, the
    ///connection is expected to succeed, and vice versa.
    void connectNodes(boost::shared_ptr<Natron::Node> input,boost::shared_ptr<Natron::Node> output,
                      int inputNumber,bool expectedReturnValue);

    ///Useful function to disconnect 2 nodes together. expectedReturnValue is expected to have the same value as the return
    ///value of the underlying disconnect call. That means that if expectedReturnValue is true, the
    ///disconnection is expected to succeed, and vice versa.
    void disconnectNodes(boost::shared_ptr<Natron::Node> input,boost::shared_ptr<Natron::Node> output,bool expectedReturnvalue);

    void registerTestPlugins();

    ///////////////Pointers to plug-ins that might be used by all the tests. This makes
    ///////////////it easy to create a node for a specific plug-in, you just have to call
    /////////////// createNode(<pluginID>).
    ///////////////If you add a test that need another plug-in, just add it to this list and in the
    ///////////////function registerTestPlugin
    ///////////////Adding a plugin here means that the user running the test MUST have the plugin
    ///////////////installed in order to the test to run, so don't add fancy plug-ins, you should
    ///////////////stick to the default plug-ins that are pre-compiled in the shell-script.
    QString _dotGeneratorPluginID;
    QString _readOIIOPluginID;
    QString _writeOIIOPluginID;
    std::vector<QString> _allTestPluginIDs;
    AppInstance* _app;
};

NATRON_NAMESPACE_EXIT

#endif // BASETEST_H
