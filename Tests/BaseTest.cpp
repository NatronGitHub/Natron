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

#include <map>
#include <iostream>
#include "Engine/OfxHost.h"
#include "Global/AppManager.h"
BaseTest::BaseTest()
    : testing::Test()
    , _ofxHost(new Natron::OfxHost)
{
    _ofxHost->loadOFXPlugins(&_plugins,NULL,NULL);
}

BaseTest::~BaseTest() {

}

void BaseTest::SetUp() {

}

void BaseTest::TearDown() {
    delete _ofxHost;
    for(unsigned int i = 0; i < _plugins.size(); ++i) {
        delete _plugins[i];
    }
}

TEST_F(BaseTest,Load) {

}
