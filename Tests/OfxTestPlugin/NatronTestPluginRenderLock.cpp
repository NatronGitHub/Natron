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

// net.natron.test.RenderLockUnsafe and net.natron.test.RenderLockInstanceSafe (Tests/OfxRenderLock_Test.cpp,
// #1614): temporal filters, kOfxImageEffectRenderUnsafe and kOfxImageEffectRenderInstanceSafe, which Natron
// renders while holding the node's render lock and, for the Unsafe one, the plug-in's render lock. When the render
// time is the value of their "fetchTime" parameter, the render action:
// - sets the counter "RenderLock.<label>.inRender" (label: Unsafe or InstanceSafe);
// - waits (10 s at most) until the counter "RenderLock.<other label>.inRender" is set (by the other plug-in, or by
//   the test), counting a timeout in "RenderLock.<label>.waitTimeout";
// - fetches the frame time + 100 of Source, which was not asked for in getFramesNeeded, so that Natron renders it
//   in clipGetImage (or waits for the thread that renders it), and counts the fetched images in
//   "RenderLock.<label>.fetched".
// The other renders fetch nothing. The renders of frames from 100 on (the fetched frames) are counted in
// "RenderLock.<label>.renderedByFetcher" if the thread of a fetching action renders them, in
// "RenderLock.<label>.renderedElsewhere" otherwise. A render action that starts on an instance while another one
// runs on it counts "RenderLock.<label>.overlap" (kOfxImageEffectRenderInstanceSafe: "any instance can have a
// single 'render' call at any one time"). Code of the render action of the Unsafe plug-in that runs while it runs
// on another thread (outside clipGetImage) counts "RenderLock.Unsafe.concurrent". The images are not written.

#include <chrono>
#include <cstring>
#include <map>
#include <mutex>
#include <set>
#include <string>
#include <thread>

#include <ofxCore.h>
#include <ofxImageEffect.h>
#include <ofxParam.h>
#include <ofxProperty.h>

#include "NatronTestPluginRegistry.h"

namespace {
OfxHost* gHost = 0;
const OfxPropertySuiteV1* gPropSuite = 0;
const OfxImageEffectSuiteV1* gEffectSuite = 0;
const OfxParameterSuiteV1* gParamSuite = 0;

const char* const kParamFetchTime = "fetchTime";
const double kFetchOffset = 100.;
const std::chrono::seconds kWaitTime(10);

std::mutex gMutex;
std::map<OfxImageEffectHandle, int> gActionsInProgress; // by instance, guarded by gMutex
std::multiset<std::thread::id> gFetchingThreads; // the threads of the fetching actions, guarded by gMutex
int gUnsafeCodeRunning = 0; // the threads running code of the Unsafe plug-in's render action, guarded by gMutex

OfxStatus
describe(OfxImageEffectHandle effect,
         const char* label,
         const char* threadSafety)
{
    OfxPropertySetHandle props;
    OfxStatus stat = gEffectSuite->getPropertySet(effect, &props);

    if (stat != kOfxStatOK) {
        return stat;
    }
    gPropSuite->propSetString(props, kOfxPropLabel, 0, label);
    gPropSuite->propSetString(props, kOfxImageEffectPluginPropGrouping, 0, "Other/Test");
    gPropSuite->propSetString(props, kOfxImageEffectPropSupportedContexts, 0, kOfxImageEffectContextFilter);
    gPropSuite->propSetString(props, kOfxImageEffectPropSupportedPixelDepths, 0, kOfxBitDepthFloat);
    gPropSuite->propSetString(props, kOfxImageEffectPluginRenderThreadSafety, 0, threadSafety);
    gPropSuite->propSetInt(props, kOfxImageEffectPluginPropSingleInstance, 0, 0);
    // it fetches other frames than the one it renders; Natron also caches the output of such an effect
    gPropSuite->propSetInt(props, kOfxImageEffectPropTemporalClipAccess, 0, 1);

    return kOfxStatOK;
}

OfxStatus
describeInContext(OfxImageEffectHandle effect)
{
    const char* clips[] = { kOfxImageEffectSimpleSourceClipName, kOfxImageEffectOutputClipName };

    for (const char* clip : clips) {
        OfxPropertySetHandle props;
        OfxStatus stat = gEffectSuite->clipDefine(effect, clip, &props);
        if (stat != kOfxStatOK) {
            return stat;
        }
        gPropSuite->propSetString(props, kOfxImageEffectPropSupportedComponents, 0, kOfxImageComponentRGBA);
    }
    OfxParamSetHandle paramSet = 0;
    OfxPropertySetHandle props;
    if ( (gEffectSuite->getParamSet(effect, &paramSet) != kOfxStatOK) ||
         (gParamSuite->paramDefine(paramSet, kOfxParamTypeDouble, kParamFetchTime, &props) != kOfxStatOK) ) {
        return kOfxStatFailed;
    }
    gPropSuite->propSetDouble(props, kOfxParamPropDefault, 0, -1.);

    return kOfxStatOK;
}

// Wait (kWaitTime at most) until the counter is set; false on timeout
bool
waitForCounter(const std::string& name)
{
    const std::chrono::steady_clock::time_point deadline = std::chrono::steady_clock::now() + kWaitTime;

    while ( (natronTestGetCounter(name) == 0) && (std::chrono::steady_clock::now() < deadline) ) {
        std::this_thread::sleep_for( std::chrono::milliseconds(1) );
    }

    return natronTestGetCounter(name) != 0;
}

// Counts the render actions in progress on an instance, while it exists
class ActionInProgress
{
public:
    ActionInProgress(OfxImageEffectHandle effect,
                     const std::string& label)
        : _effect(effect)
    {
        std::lock_guard<std::mutex> l(gMutex);
        if (++gActionsInProgress[_effect] > 1) {
            natronTestIncrementCounter("RenderLock." + label + ".overlap");
        }
    }

    ActionInProgress(const ActionInProgress&) = delete;
    ActionInProgress& operator=(const ActionInProgress&) = delete;

    ~ActionInProgress()
    {
        std::lock_guard<std::mutex> l(gMutex);
        --gActionsInProgress[_effect];
    }

private:
    const OfxImageEffectHandle _effect;
};

// Counts the threads running code of the render action of the Unsafe plug-in, while it exists (pause() around the
// host calls that may render upstream)
class UnsafeCodeRunning
{
public:
    explicit UnsafeCodeRunning(bool unsafe)
        : _unsafe(unsafe)
        , _running(false)
    {
        resume();
    }

    UnsafeCodeRunning(const UnsafeCodeRunning&) = delete;
    UnsafeCodeRunning& operator=(const UnsafeCodeRunning&) = delete;

    ~UnsafeCodeRunning()
    {
        pause();
    }

    void pause()
    {
        if (_running) {
            std::lock_guard<std::mutex> l(gMutex);
            --gUnsafeCodeRunning;
            _running = false;
        }
    }

    void resume()
    {
        if (_unsafe && !_running) {
            std::lock_guard<std::mutex> l(gMutex);
            if (++gUnsafeCodeRunning > 1) {
                natronTestIncrementCounter("RenderLock.Unsafe.concurrent");
            }
            _running = true;
        }
    }

private:
    const bool _unsafe;
    bool _running;
};

// Marks the thread as the one of a fetching action, while it exists
class FetchingThread
{
public:
    FetchingThread()
    {
        std::lock_guard<std::mutex> l(gMutex);
        gFetchingThreads.insert( std::this_thread::get_id() );
    }

    FetchingThread(const FetchingThread&) = delete;
    FetchingThread& operator=(const FetchingThread&) = delete;

    ~FetchingThread()
    {
        std::lock_guard<std::mutex> l(gMutex);
        gFetchingThreads.erase( gFetchingThreads.find( std::this_thread::get_id() ) );
    }

    static bool isCurrent()
    {
        std::lock_guard<std::mutex> l(gMutex);

        return gFetchingThreads.count( std::this_thread::get_id() ) != 0;
    }
};

OfxStatus
render(OfxImageEffectHandle effect,
       OfxPropertySetHandle inArgs,
       const std::string& label,
       const std::string& otherLabel)
{
    ActionInProgress inProgress(effect, label);
    UnsafeCodeRunning running(label == "Unsafe");
    OfxTime time = 0.;
    gPropSuite->propGetDouble(inArgs, kOfxPropTime, 0, &time);
    if (time >= kFetchOffset) {
        natronTestIncrementCounter( "RenderLock." + label + ( FetchingThread::isCurrent() ? ".renderedByFetcher" : ".renderedElsewhere" ) );
    }
    OfxParamSetHandle paramSet = 0;
    OfxParamHandle param = 0;
    double fetchTime = -1.;
    if ( (gEffectSuite->getParamSet(effect, &paramSet) != kOfxStatOK) ||
         (gParamSuite->paramGetHandle(paramSet, kParamFetchTime, &param, 0) != kOfxStatOK) ||
         (gParamSuite->paramGetValueAtTime(param, time, &fetchTime) != kOfxStatOK) ) {
        return kOfxStatFailed;
    }
    if (time != fetchTime) {
        return kOfxStatOK;
    }
    natronTestSetCounter("RenderLock." + label + ".inRender", 1);
    if ( !waitForCounter("RenderLock." + otherLabel + ".inRender") ) {
        natronTestIncrementCounter("RenderLock." + label + ".waitTimeout");
    }
    OfxImageClipHandle source = 0;
    if (gEffectSuite->clipGetHandle(effect, kOfxImageEffectSimpleSourceClipName, &source, 0) != kOfxStatOK) {
        return kOfxStatFailed;
    }
    FetchingThread fetching;
    OfxPropertySetHandle image = 0;
    running.pause(); // Natron may run other renders of the plug-in while this one waits in clipGetImage
    const OfxStatus fetchStatus = gEffectSuite->clipGetImage(source, time + kFetchOffset, 0, &image);
    running.resume();
    if (fetchStatus == kOfxStatOK) {
        natronTestIncrementCounter("RenderLock." + label + ".fetched");
        gEffectSuite->clipReleaseImage(image);
    }

    return kOfxStatOK;
}

OfxStatus
pluginMain(const char* action,
           OfxImageEffectHandle effect,
           OfxPropertySetHandle inArgs,
           OfxPropertySetHandle outArgs,
           bool unsafe)
{
    const char* label = unsafe ? "Unsafe" : "InstanceSafe";

    if (std::strcmp(action, kOfxActionLoad) == 0) {
        if (!gHost) {
            return kOfxStatErrMissingHostFeature;
        }
        gPropSuite = (const OfxPropertySuiteV1*)gHost->fetchSuite(gHost->host, kOfxPropertySuite, 1);
        gEffectSuite = (const OfxImageEffectSuiteV1*)gHost->fetchSuite(gHost->host, kOfxImageEffectSuite, 1);
        gParamSuite = (const OfxParameterSuiteV1*)gHost->fetchSuite(gHost->host, kOfxParameterSuite, 1);

        return (gPropSuite && gEffectSuite && gParamSuite) ? kOfxStatOK : kOfxStatErrMissingHostFeature;
    }
    if (std::strcmp(action, kOfxActionDescribe) == 0) {
        return describe(effect, unsafe ? "RenderLockUnsafeTest" : "RenderLockInstanceSafeTest",
                        unsafe ? kOfxImageEffectRenderUnsafe : kOfxImageEffectRenderInstanceSafe);
    }
    if (std::strcmp(action, kOfxImageEffectActionDescribeInContext) == 0) {
        return describeInContext(effect);
    }
    if (std::strcmp(action, kOfxImageEffectActionGetClipPreferences) == 0) {
        // each frame is a different image
        gPropSuite->propSetInt(outArgs, kOfxImageEffectFrameVarying, 0, 1);

        return kOfxStatOK;
    }
    if (std::strcmp(action, kOfxImageEffectActionRender) == 0) {
        return render(effect, inArgs, label, unsafe ? "InstanceSafe" : "Unsafe");
    }

    return kOfxStatReplyDefault;
}

OfxStatus
unsafeMain(const char* action,
           const void* handle,
           OfxPropertySetHandle inArgs,
           OfxPropertySetHandle outArgs)
{
    return pluginMain(action, (OfxImageEffectHandle)handle, inArgs, outArgs, true);
}

OfxStatus
instanceSafeMain(const char* action,
                 const void* handle,
                 OfxPropertySetHandle inArgs,
                 OfxPropertySetHandle outArgs)
{
    return pluginMain(action, (OfxImageEffectHandle)handle, inArgs, outArgs, false);
}

void
setHost(OfxHost* host)
{
    gHost = host;
}

OfxPlugin gUnsafePlugin = {
    kOfxImageEffectPluginApi, 1, "net.natron.test.RenderLockUnsafe", 1, 0, setHost, unsafeMain
};
OfxPlugin gInstanceSafePlugin = {
    kOfxImageEffectPluginApi, 1, "net.natron.test.RenderLockInstanceSafe", 1, 0, setHost, instanceSafeMain
};
NatronTestPluginRegistrar gUnsafeRegistrar(&gUnsafePlugin);
NatronTestPluginRegistrar gInstanceSafeRegistrar(&gInstanceSafePlugin);
} // anonymous namespace
