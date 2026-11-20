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

// The key functions of the parameter suite and the indices of the parametric parameter suite (OFX 1.5.1,
// #1385), on the parameters of the test plug-in net.natron.test.Params (Tests/OfxTestPlugin):
// - paramGetKeyIndex: "< 0 indicates search for the previous key before the indicated time", and the
//   index is "set to -1 if no key was found" (ofxParam.h);
// - paramDeleteKey: "kOfxStatErrBadIndex - no key at the given time";
// - the parametric suite: "kOfxStatErrBadIndex - the curve index was invalid", and "the curve index or
//   the control point index was invalid" (ofxParametricParam.h).

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <gtest/gtest.h>

#include <ofxCore.h>
#include <ofxParam.h>
#include <ofxParametricParam.h>
#include <ofxhImageEffect.h>

#include "Engine/AppManager.h"
#include "Engine/Node.h"

#include "OfxTestPlugin.h"

NATRON_NAMESPACE_USING

namespace {
class OfxParamKeysTest
    : public ::testing::Test
{
protected:
    NodePtr _node;

    virtual void SetUp() OVERRIDE
    {
        if ( NatronTestPlugin::isAvailable(kNatronTestPluginParams) ) {
            _node = NatronTestPlugin::createNode(kNatronTestPluginParams);
        }
    }

    virtual void TearDown() OVERRIDE
    {
        if (_node) {
            _node->destroyNode(true /*blockingDestroy*/, false /*autoReconnect*/);
            _node.reset();
        }
    }

    // the handle of a parameter, or NULL
    OfxParamHandle param(const char* name)
    {
        OFX::Host::Param::Instance* p = NatronTestPlugin::ofxParam(_node, name);

        return p ? (OfxParamHandle)p->getHandle() : 0;
    }

    bool skipped()
    {
        if (!_node) {
            NatronTestPlugin::reportSkipped(kNatronTestPluginParams);

            return true;
        }

        return false;
    }
};
} // anonymous namespace

TEST_F(OfxParamKeysTest, GetKeyIndex)
{
    if ( skipped() ) {
        return;
    }
    const OfxParameterSuiteV1* suite = NatronTestPlugin::parameterSuite();
    OfxParamHandle p = param("doubleFrom");
    ASSERT_TRUE(suite && p);
    ASSERT_EQ( kOfxStatOK, suite->paramSetValueAtTime(p, 1., 0.25) );
    ASSERT_EQ( kOfxStatOK, suite->paramSetValueAtTime(p, 5., 0.75) );
    unsigned int nKeys = 0;
    ASSERT_EQ( kOfxStatOK, suite->paramGetNumKeys(p, &nKeys) );
    ASSERT_EQ(2u, nKeys);

    int index = 1234;
    // at a time
    EXPECT_EQ( kOfxStatOK, suite->paramGetKeyIndex(p, 5., 0, &index) );
    EXPECT_EQ(1, index);
    index = 1234;
    EXPECT_EQ( kOfxStatFailed, suite->paramGetKeyIndex(p, 3., 0, &index) );
    EXPECT_EQ(-1, index);
    // before: the previous key of a key is found too (it used to fail)
    index = 1234;
    EXPECT_EQ( kOfxStatOK, suite->paramGetKeyIndex(p, 5., -1, &index) );
    EXPECT_EQ(0, index);
    index = 1234;
    EXPECT_EQ( kOfxStatOK, suite->paramGetKeyIndex(p, 3., -1, &index) );
    EXPECT_EQ(0, index);
    index = 1234;
    EXPECT_EQ( kOfxStatOK, suite->paramGetKeyIndex(p, 10., -1, &index) );
    EXPECT_EQ(1, index);
    index = 1234;
    EXPECT_EQ( kOfxStatFailed, suite->paramGetKeyIndex(p, 1., -1, &index) );
    EXPECT_EQ(-1, index);
    // after
    index = 1234;
    EXPECT_EQ( kOfxStatOK, suite->paramGetKeyIndex(p, 1., 1, &index) );
    EXPECT_EQ(1, index);
    index = 1234;
    EXPECT_EQ( kOfxStatOK, suite->paramGetKeyIndex(p, 0., 1, &index) );
    EXPECT_EQ(0, index);
    index = 1234;
    EXPECT_EQ( kOfxStatFailed, suite->paramGetKeyIndex(p, 5., 1, &index) );
    EXPECT_EQ(-1, index);
}

TEST_F(OfxParamKeysTest, DeleteKey)
{
    if ( skipped() ) {
        return;
    }
    const OfxParameterSuiteV1* suite = NatronTestPlugin::parameterSuite();
    OfxParamHandle p = param("doubleFrom");
    ASSERT_TRUE(suite && p);
    ASSERT_EQ( kOfxStatOK, suite->paramSetValueAtTime(p, 1., 0.25) );
    ASSERT_EQ( kOfxStatOK, suite->paramSetValueAtTime(p, 5., 0.75) );

    EXPECT_EQ( kOfxStatErrBadIndex, suite->paramDeleteKey(p, 3.) );
    unsigned int nKeys = 0;
    EXPECT_EQ( kOfxStatOK, suite->paramGetNumKeys(p, &nKeys) );
    EXPECT_EQ(2u, nKeys);
    EXPECT_EQ( kOfxStatOK, suite->paramDeleteKey(p, 5.) );
    EXPECT_EQ( kOfxStatOK, suite->paramGetNumKeys(p, &nKeys) );
    EXPECT_EQ(1u, nKeys);
    EXPECT_EQ( kOfxStatErrBadIndex, suite->paramDeleteKey(p, 5.) );
}

#ifdef OFX_SUPPORTS_PARAMETRIC
TEST_F(OfxParamKeysTest, ParametricBadIndex)
{
    if ( skipped() ) {
        return;
    }
    ::OfxHost* host = OFX::Host::ImageEffect::gImageEffectHost->getHandle();
    const OfxParametricParameterSuiteV1* suite = (const OfxParametricParameterSuiteV1*)host->fetchSuite(host->host, kOfxParametricParameterSuite, 1);
    OfxParamHandle p = param("parametricFrom"); // one curve
    ASSERT_TRUE(suite && p);

    double value = 0.;
    int n = -1;
    double key = 0., keyValue = 0.;
    ASSERT_EQ( kOfxStatOK, suite->parametricParamDeleteAllControlPoints(p, 0) );
    EXPECT_EQ( kOfxStatOK, suite->parametricParamAddControlPoint(p, 0, 0., 0.5, 0.5, false) );
    EXPECT_EQ( kOfxStatOK, suite->parametricParamGetNControlPoints(p, 0, 0., &n) );
    EXPECT_EQ(1, n);
    EXPECT_EQ( kOfxStatOK, suite->parametricParamGetValue(p, 0, 0., 0.5, &value) );
    EXPECT_EQ( kOfxStatOK, suite->parametricParamGetNthControlPoint(p, 0, 0., 0, &key, &keyValue) );

    // curve index 1 and -1 don't exist
    for (int curve = -1; curve <= 1; curve += 2) {
        EXPECT_EQ( kOfxStatErrBadIndex, suite->parametricParamGetValue(p, curve, 0., 0.5, &value) ) << curve;
        EXPECT_EQ( kOfxStatErrBadIndex, suite->parametricParamGetNControlPoints(p, curve, 0., &n) ) << curve;
        EXPECT_EQ( kOfxStatErrBadIndex, suite->parametricParamGetNthControlPoint(p, curve, 0., 0, &key, &keyValue) ) << curve;
        EXPECT_EQ( kOfxStatErrBadIndex, suite->parametricParamSetNthControlPoint(p, curve, 0., 0, 0.5, 0.5, false) ) << curve;
        EXPECT_EQ( kOfxStatErrBadIndex, suite->parametricParamAddControlPoint(p, curve, 0., 0.25, 0.25, false) ) << curve;
        EXPECT_EQ( kOfxStatErrBadIndex, suite->parametricParamDeleteControlPoint(p, curve, 0) ) << curve;
        EXPECT_EQ( kOfxStatErrBadIndex, suite->parametricParamDeleteAllControlPoints(p, curve) ) << curve;
    }
    // control point 1 and -1 don't exist
    for (int nth = -1; nth <= 1; nth += 2) {
        EXPECT_EQ( kOfxStatErrBadIndex, suite->parametricParamGetNthControlPoint(p, 0, 0., nth, &key, &keyValue) ) << nth;
        EXPECT_EQ( kOfxStatErrBadIndex, suite->parametricParamSetNthControlPoint(p, 0, 0., nth, 0.5, 0.5, false) ) << nth;
        EXPECT_EQ( kOfxStatErrBadIndex, suite->parametricParamDeleteControlPoint(p, 0, nth) ) << nth;
    }
    EXPECT_EQ( kOfxStatOK, suite->parametricParamGetNControlPoints(p, 0, 0., &n) );
    EXPECT_EQ(1, n);
}

#endif
