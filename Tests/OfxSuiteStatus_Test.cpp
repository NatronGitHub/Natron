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

// The return codes of the OFX suites that Natron implements (OFX 1.5.1, #1385): the host's kOfxPropName
// (ofxProperty.h), the multithread suite (ofxMultiThread.h), the message suite (ofxMessage.h), the
// progress suite (ofxProgress.h), the timeline suite (ofxTimeLine.h) and flushResources (ofxGPURender.h).

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <atomic>
#include <cmath>
#include <limits>
#include <list>
#include <vector>

#include <gtest/gtest.h>

#include <ofxCore.h>
#include <ofxGPURender.h>
#include <ofxMessage.h>
#include <ofxMultiThread.h>
#include <ofxParam.h>
#include <ofxProgress.h>
#include <ofxProperty.h>
#include <ofxTimeLine.h>
#include <ofxhImageEffect.h>

#include "Engine/AppInstance.h"
#include "Engine/AppManager.h"
#include "Engine/KnobTypes.h"
#include "Engine/LogEntry.h"
#include "Engine/Node.h"
#include "Engine/OfxEffectInstance.h"
#include "Engine/OfxHost.h"
#include "Engine/TimeLine.h"

#include "OfxTestPlugin.h"

NATRON_NAMESPACE_USING

namespace {
const void*
fetchSuite(const char* name,
           int version)
{
    ::OfxHost* handle = OFX::Host::ImageEffect::gImageEffectHost->getHandle();

    return handle->fetchSuite(handle->host, name, version);
}

const OfxMultiThreadSuiteV1*
multiThreadSuite()
{
    return (const OfxMultiThreadSuiteV1*)fetchSuite(kOfxMultiThreadSuite, 1);
}

// what the thread functions below see
struct ThreadCheck
{
    std::atomic<int> badIndex;
    std::atomic<int> notSpawned;
    std::atomic<int> nestedStatus; // the status of a nested multiThread call, or 0
    bool nest;

    ThreadCheck()
        : badIndex(0)
        , notSpawned(0)
        , nestedStatus(0)
        , nest(false)
    {
    }
};

void
nestedFunction(unsigned int /*threadIndex*/,
               unsigned int /*threadMax*/,
               void* /*customArg*/)
{
}

void
checkFunction(unsigned int threadIndex,
              unsigned int /*threadMax*/,
              void* customArg)
{
    ThreadCheck* check = (ThreadCheck*)customArg;
    const OfxMultiThreadSuiteV1* suite = multiThreadSuite();
    unsigned int index = 9999;

    if ( (suite->multiThreadIndex(&index) != kOfxStatOK) || (index != threadIndex) ) {
        ++check->badIndex;
    }
    if (suite->multiThreadIsSpawnedThread() != 1) {
        ++check->notSpawned;
    }
    if (check->nest) {
        check->nestedStatus = suite->multiThread(nestedFunction, 2, 0);
    }
}

// an OFX node of the test plug-in net.natron.test.Params, and its effect instance handle
class OfxSuiteStatusTest
    : public ::testing::Test
{
protected:
    NodePtr _node;
    OfxImageEffectHandle _effect;

    OfxSuiteStatusTest()
        : _node()
        , _effect(0)
    {
    }

    virtual void SetUp() OVERRIDE
    {
        if ( !NatronTestPlugin::isAvailable(kNatronTestPluginParams) ) {
            return;
        }
        _node = NatronTestPlugin::createNode(kNatronTestPluginParams);
        OfxEffectInstance* effect = _node ? dynamic_cast<OfxEffectInstance*>( _node->getEffectInstance().get() ) : 0;
        if ( effect && effect->effectInstance() ) {
            _effect = effect->effectInstance()->getHandle();
        }
    }

    virtual void TearDown() OVERRIDE
    {
        if (_node) {
            _node->destroyNode(true /*blockingDestroy*/, false /*autoReconnect*/);
            _node.reset();
        }
    }

    bool skipped()
    {
        if (!_effect) {
            NatronTestPlugin::reportSkipped(kNatronTestPluginParams);

            return true;
        }

        return false;
    }
};
} // anonymous namespace

// ofxProperty.h getters: "kOfxStatErrBadIndex - if the property index is invalid". The host's kOfxPropName
// is computed by a get hook (the host name proxy of OfxHost::getStringProperty()).
TEST(OfxSuiteStatus, HostNameBadIndex)
{
    ::OfxHost* handle = OFX::Host::ImageEffect::gImageEffectHost->getHandle();
    const OfxPropertySuiteV1* props = NatronTestPlugin::propertySuite();

    ASSERT_TRUE(props != 0);
    const char* name = 0;
    // (index 0 is not read here: outside an action, the host name hook uses the last effect that called
    // an action on this thread, which an earlier test may have destroyed)
    EXPECT_EQ( kOfxStatErrBadIndex, props->propGetString(handle->host, kOfxPropName, 1, &name) );
}

// propGetDimension: "kOfxStatErrBadHandle - if the handle was invalid" (the magic number of the property set,
// NatronGitHub/openfx#200)
TEST(OfxSuiteStatus, PropGetDimensionBadHandle)
{
    const OfxPropertySuiteV1* props = NatronTestPlugin::propertySuite();

    ASSERT_TRUE(props != 0);
    std::vector<unsigned char> notASet(256, 0);
    int count = -1;
    EXPECT_EQ( kOfxStatErrBadHandle, props->propGetDimension( (OfxPropertySetHandle)&notASet[0], kOfxPropName, &count ) );
    ::OfxHost* handle = OFX::Host::ImageEffect::gImageEffectHost->getHandle();
    EXPECT_EQ( kOfxStatOK, props->propGetDimension(handle->host, kOfxPropName, &count) );
    EXPECT_EQ(1, count);
}

// mutexDestroy, mutexLock, mutexUnLock, mutexTryLock: "kOfxStatErrBadHandle - if the handle was bad"
TEST(OfxSuiteStatus, MutexBadHandle)
{
    const OfxMultiThreadSuiteV1* suite = multiThreadSuite();

    ASSERT_TRUE(suite != 0);
    // not a mutex of the suite
    std::vector<unsigned char> notAMutex(256, 0);
    OfxMutexHandle bad = (OfxMutexHandle)&notAMutex[0];
    EXPECT_EQ( kOfxStatErrBadHandle, suite->mutexLock(bad) );
    EXPECT_EQ( kOfxStatErrBadHandle, suite->mutexUnLock(bad) );
    EXPECT_EQ( kOfxStatErrBadHandle, suite->mutexTryLock(bad) );
    EXPECT_EQ( kOfxStatErrBadHandle, suite->mutexDestroy(bad) );

    OfxMutexHandle m = 0;
    ASSERT_EQ( kOfxStatOK, suite->mutexCreate(&m, 0) );
    EXPECT_EQ( kOfxStatOK, suite->mutexLock(m) );
    EXPECT_EQ( kOfxStatOK, suite->mutexUnLock(m) );
    EXPECT_EQ( kOfxStatOK, suite->mutexDestroy(m) );
}

// multiThreadIndex "returns the thread index, which is the same as the threadIndex argument passed to the
// OfxThreadFunctionV1", and multiThreadIsSpawnedThread is 1 in the thread functions, also when the host runs
// them one after the other in the calling thread (a single thread). Outside, the index is 0 and the thread
// is not spawned.
TEST(OfxSuiteStatus, MultiThreadIndex)
{
    const OfxMultiThreadSuiteV1* suite = multiThreadSuite();

    ASSERT_TRUE(suite != 0);
    for (unsigned int nThreads = 1; nThreads <= 4; ++nThreads) {
        ThreadCheck check;
        EXPECT_EQ( kOfxStatOK, suite->multiThread(checkFunction, nThreads, &check) );
        EXPECT_EQ(0, check.badIndex) << nThreads << " threads";
        EXPECT_EQ(0, check.notSpawned) << nThreads << " threads";
    }
    unsigned int index = 9999;
    EXPECT_EQ( kOfxStatOK, suite->multiThreadIndex(&index) );
    EXPECT_EQ(0u, index);
    EXPECT_EQ( 0, suite->multiThreadIsSpawnedThread() );
}

// "This function cannot be called recursively": "kOfxStatErrExists, failed in an attempt to call multiThread
// recursively"
TEST(OfxSuiteStatus, MultiThreadRecursive)
{
    const OfxMultiThreadSuiteV1* suite = multiThreadSuite();

    ASSERT_TRUE(suite != 0);
    for (unsigned int nThreads = 1; nThreads <= 2; ++nThreads) {
        ThreadCheck check;
        check.nest = true;
        EXPECT_EQ( kOfxStatOK, suite->multiThread(checkFunction, nThreads, &check) );
        EXPECT_EQ(kOfxStatErrExists, check.nestedStatus) << nThreads << " threads";
    }
}

// message: "kOfxStatOK - if the message was successfully posted", "kOfxStatFailed - if the message could
// not be posted for some reason"
TEST_F(OfxSuiteStatusTest, MessageStatus)
{
    if ( skipped() ) {
        return;
    }
    const OfxMessageSuiteV1* suite = (const OfxMessageSuiteV1*)fetchSuite(kOfxMessageSuite, 1);
    ASSERT_TRUE(suite != 0);
    EXPECT_EQ( kOfxStatOK, suite->message(_effect, kOfxMessageLog, 0, "OfxSuiteStatus: %s", "log") );
    EXPECT_EQ( kOfxStatOK, suite->message(_effect, kOfxMessageWarning, 0, "OfxSuiteStatus: %s", "warning") );
    EXPECT_EQ( kOfxStatOK, suite->message(_effect, kOfxMessageError, 0, "OfxSuiteStatus: %s", "error") );
    EXPECT_EQ( kOfxStatFailed, suite->message(_effect, "NotAMessageType", 0, "OfxSuiteStatus: %s", "unknown") );
    // the arguments are formatted (they were passed as one va_list argument, which printed garbage)
    appPTR->clearErrorLog_mt_safe();
    EXPECT_EQ( kOfxStatOK, suite->message(_effect, kOfxMessageLog, 0, "OfxSuiteStatus: %s %d", "formatted", 42) );
    EXPECT_EQ( kOfxStatOK, suite->message(0, kOfxMessageLog, 0, "OfxSuiteStatus: %s %d", "host", 43) );
    std::list<LogEntry> entries;
    appPTR->getErrorLog_mt_safe(&entries);
    bool foundEffect = false, foundHost = false;
    for (std::list<LogEntry>::const_iterator it = entries.begin(); it != entries.end(); ++it) {
        foundEffect = foundEffect || it->message.contains( QString::fromUtf8("OfxSuiteStatus: formatted 42") );
        foundHost = foundHost || it->message.contains( QString::fromUtf8("OfxSuiteStatus: host 43") );
    }
    EXPECT_TRUE(foundEffect);
    EXPECT_TRUE(foundHost);
    // a host-level message (NULL handle) goes to the log
    EXPECT_EQ( kOfxStatOK, suite->message(0, kOfxMessageLog, 0, "OfxSuiteStatus: %s", "host log") );
    EXPECT_EQ( kOfxStatFailed, suite->message(0, "NotAMessageType", 0, "OfxSuiteStatus: %s", "host unknown") );
}

// progressUpdate: "kOfxStatErrBadHandle - the progress handle was invalid": there is no progress task to
// update before progressStart. Natron's progress panel answered kOfxStatReplyNo, which means "abort".
TEST_F(OfxSuiteStatusTest, ProgressUpdateWithoutStart)
{
    if ( skipped() ) {
        return;
    }
    const OfxProgressSuiteV1* suite = (const OfxProgressSuiteV1*)fetchSuite(kOfxProgressSuite, 1);
    ASSERT_TRUE(suite != 0);
    OfxStatus stat = suite->progressUpdate(_effect, 0.5);
    EXPECT_NE(kOfxStatReplyNo, stat);
    EXPECT_EQ(kOfxStatErrBadHandle, stat);

    EXPECT_EQ( kOfxStatOK, suite->progressStart(_effect, "OfxSuiteStatus") );
    EXPECT_EQ( kOfxStatOK, suite->progressUpdate(_effect, 0.5) );
    EXPECT_EQ( kOfxStatOK, suite->progressEnd(_effect) );
    EXPECT_EQ( kOfxStatErrBadHandle, suite->progressUpdate(_effect, 0.5) );
}

// gotoTime "moves the timeline to time": Natron's timeline has integer frames, so it goes to the nearest one
// (it used to truncate). An illegal time gives kOfxStatErrValue or leaves the timeline alone.
TEST_F(OfxSuiteStatusTest, GotoTime)
{
    if ( skipped() ) {
        return;
    }
    const OfxTimeLineSuiteV1* suite = (const OfxTimeLineSuiteV1*)fetchSuite(kOfxTimeLineSuite, 1);
    ASSERT_TRUE(suite != 0);
    TimeLinePtr timeline = _node->getApp()->getTimeLine();
    const SequenceTime startFrame = timeline->currentFrame();

    EXPECT_EQ( kOfxStatOK, suite->gotoTime(_effect, 4.9999999) );
    EXPECT_EQ( 5, timeline->currentFrame() );
    double time = 0.;
    EXPECT_EQ( kOfxStatOK, suite->getTime(_effect, &time) );
    EXPECT_EQ(5., time);
    EXPECT_EQ( kOfxStatOK, suite->gotoTime(_effect, -2.6) );
    EXPECT_EQ( -3, timeline->currentFrame() );
    EXPECT_EQ( kOfxStatErrValue, suite->gotoTime( _effect, std::numeric_limits<double>::quiet_NaN() ) );
    EXPECT_EQ( -3, timeline->currentFrame() );
    // an infinite or huge time has no frame: "an illegal value", and the timeline does not move
    EXPECT_EQ( kOfxStatErrValue, suite->gotoTime( _effect, std::numeric_limits<double>::infinity() ) );
    EXPECT_EQ( -3, timeline->currentFrame() );
    EXPECT_EQ( kOfxStatErrValue, suite->gotoTime(_effect, 1e300) );
    EXPECT_EQ( -3, timeline->currentFrame() );

    timeline->seekFrame(startFrame, false, 0, eTimelineChangeReasonOtherSeek);
}

#ifdef OFX_SUPPORTS_OPENGLRENDER
// flushResources: "kOfxStatOK - the host has actually released some resources", "kOfxStatReplyDefault -
// nothing the host could do": Natron releases nothing, so the plug-in must not retry its allocation.
TEST(OfxSuiteStatus, FlushResources)
{
    const OfxImageEffectOpenGLRenderSuiteV1* suite = (const OfxImageEffectOpenGLRenderSuiteV1*)fetchSuite(kOfxOpenGLRenderSuite, 1);

    ASSERT_TRUE(suite != 0);
    EXPECT_EQ( kOfxStatReplyDefault, suite->flushResources() );
}

#endif

// The parameter properties that ofxParam.h documents as "instance (read only)" are plug-in read-only on
// instances (NatronGitHub/openfx#205), but Natron's read-only policy (ePluginReadOnlyWarn, Engine/OfxHost.cpp,
// for old plug-ins) still accepts a write, with a warning. kOfxParamPropGroupOpen stays writable on instances
// (a Natron extension, ofxNatron.h), and a write opens or closes the group knob (OfxGroupInstance::setOpen()).
TEST_F(OfxSuiteStatusTest, InstanceReadOnlyParamProperties)
{
    if ( skipped() ) {
        return;
    }
    const OfxParameterSuiteV1* params = NatronTestPlugin::parameterSuite();
    const OfxPropertySuiteV1* props = NatronTestPlugin::propertySuite();
    OFX::Host::Param::Instance* value = NatronTestPlugin::ofxParam(_node, "doubleFrom");
    ASSERT_TRUE(params && props && value);
    OfxPropertySetHandle valueProps = 0;
    ASSERT_EQ( kOfxStatOK, params->paramGetPropertySet( (OfxParamHandle)value->getHandle(), &valueProps ) );
    EXPECT_EQ( kOfxStatOK, props->propSetInt(valueProps, kOfxParamPropAnimates, 0, 0) );

    OFX::Host::Param::Instance* group = NatronTestPlugin::ofxParam(_node, "group");
    KnobGroupPtr groupKnob = std::dynamic_pointer_cast<KnobGroup>( _node->getKnobByName("group") );
    ASSERT_TRUE(group && groupKnob);
    OfxPropertySetHandle groupProps = 0;
    ASSERT_EQ( kOfxStatOK, params->paramGetPropertySet( (OfxParamHandle)group->getHandle(), &groupProps ) );
    EXPECT_EQ( kOfxStatOK, props->propSetInt(groupProps, kOfxParamPropGroupOpen, 0, 0) );
    EXPECT_FALSE( groupKnob->getValue() );
    EXPECT_EQ( kOfxStatOK, props->propSetInt(groupProps, kOfxParamPropGroupOpen, 0, 1) );
    EXPECT_TRUE( groupKnob->getValue() );
}
