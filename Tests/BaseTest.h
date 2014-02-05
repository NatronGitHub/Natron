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
#include <gtest/gtest.h>

namespace Natron{
class OfxHost;
class Plugin;
}
class BaseTest : public testing::Test
{
public:
    BaseTest();

    virtual ~BaseTest();

protected:

    virtual void SetUp();

    virtual void TearDown();

private:
    Natron::OfxHost* _ofxHost;
    std::vector<Natron::Plugin*> _plugins;
};

#endif // BASETEST_H
