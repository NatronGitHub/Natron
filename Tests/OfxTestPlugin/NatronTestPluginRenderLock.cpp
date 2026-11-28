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

// net.natron.test.RenderLockUnsafe, net.natron.test.RenderLockInstanceSafe and net.natron.test.RenderLockUnsafeGL
// (Tests/OfxRenderLock_Test.cpp, #1614, #1998): temporal filters, kOfxImageEffectRenderUnsafe,
// kOfxImageEffectRenderInstanceSafe, and kOfxImageEffectRenderUnsafe with an OpenGL render (and a CPU render), which
// Natron renders while holding the node's render lock and, for the Unsafe ones, the plug-in's render lock. Their
// labels (in the counter names) are Unsafe, InstanceSafe and UnsafeGL. When the render time is the value of their
// "fetchTime" parameter, the render action:
// - sets the counter "RenderLock.<label>.inRender";
// - waits (10 s at most) until the counter "RenderLock.<waitFor>.inRender" is set (by another plug-in, or by the
//   test), counting a timeout in "RenderLock.<label>.waitTimeout". waitFor is the "waitFor" parameter, or by
//   default InstanceSafe for the Unsafe plug-ins and Unsafe for the Instance-safe one. If "waitFor" is "-", the
//   action neither sets its counter nor waits;
// - fetches the frame time + 100 of Source, which was not asked for in getFramesNeeded, so that Natron renders it
//   in clipGetImage (or clipLoadTexture in the OpenGL render), or waits for the thread that renders it, and counts
//   the fetched images in "RenderLock.<label>.fetched". The OpenGL render counts the fetches that returned
//   kOfxStatErrMemory in "RenderLock.<label>.textureErrMemory" (Natron then renders the frame again on the CPU).
// The other renders fetch nothing. The OpenGL renders are counted in "RenderLock.<label>.glRenders".
// RenderLockUnsafeGL asks for an empty region of Source in getRegionsOfInterest, so that Natron never renders its
// input before the action (an OpenGL output is not cached, so a test can't render it before), and fetches the
// region (0, 0, 8, 8) of Source.
// The renders of frames from 100 on (the fetched frames) are counted in "RenderLock.<label>.renderedByFetcher" if
// the thread of a fetching action renders them, in "RenderLock.<label>.renderedElsewhere" otherwise. A render action
// that starts on an instance while another one runs on it counts "RenderLock.<label>.overlap"
// (kOfxImageEffectRenderInstanceSafe: "any instance can have a single 'render' call at any one time"). Code of the
// render action of an Unsafe plug-in that runs while it runs on another thread (outside the fetch) counts
// "RenderLock.<label>.concurrent". The images are not written.

#include <chrono>
#include <cstring>
#include <map>
#include <mutex>
#include <set>
#include <string>
#include <thread>

#include <ofxCore.h>
#include <ofxImageEffect.h>
#include <ofxGPURender.h>
#include <ofxParam.h>
#include <ofxProperty.h>

#include "NatronTestPluginRegistry.h"

namespace {
OfxHost* gHost = 0;
const OfxPropertySuiteV1* gPropSuite = 0;
const OfxImageEffectSuiteV1* gEffectSuite = 0;
const OfxParameterSuiteV1* gParamSuite = 0;
const OfxImageEffectOpenGLRenderSuiteV1* gGLSuite = 0;

const char* const kParamFetchTime = "fetchTime";
const char* const kParamWaitFor = "waitFor";
const double kFetchOffset = 100.;
const std::chrono::seconds kWaitTime(10);
const OfxRectD kGLFetchRegion = { 0., 0., 8., 8. };

// one of the plug-ins
struct PluginKind
{
    const char* label;
    const char* defaultWaitFor;
    const char* threadSafety;
    bool openGL;

    bool isUnsafe() const
    {
        return std::strcmp(threadSafety, kOfxImageEffectRenderUnsafe) == 0;
    }
};

const PluginKind kUnsafe = { "Unsafe", "InstanceSafe", kOfxImageEffectRenderUnsafe, false };
const PluginKind kInstanceSafe = { "InstanceSafe", "Unsafe", kOfxImageEffectRenderInstanceSafe, false };
const PluginKind kUnsafeGL = { "UnsafeGL", "InstanceSafe", kOfxImageEffectRenderUnsafe, true };

std::mutex gMutex;
std::map<OfxImageEffectHandle, int> gActionsInProgress; // by instance, guarded by gMutex
std::multiset<std::thread::id> gFetchingThreads; // the threads of the fetching actions, guarded by gMutex
std::map<std::string, int> gUnsafeCodeRunning; // by label, the threads running code of an Unsafe plug-in's render action, guarded by gMutex

OfxStatus
describe(OfxImageEffectHandle effect,
         const PluginKind& kind)
{
    OfxPropertySetHandle props;
    OfxStatus stat = gEffectSuite->getPropertySet(effect, &props);

    if (stat != kOfxStatOK) {
        return stat;
    }
    const std::string label = std::string("RenderLock") + kind.label + "Test";
    gPropSuite->propSetString(props, kOfxPropLabel, 0, label.c_str());
    gPropSuite->propSetString(props, kOfxImageEffectPluginPropGrouping, 0, "Other/Test");
    gPropSuite->propSetString(props, kOfxImageEffectPropSupportedContexts, 0, kOfxImageEffectContextFilter);
    gPropSuite->propSetString(props, kOfxImageEffectPropSupportedPixelDepths, 0, kOfxBitDepthFloat);
    gPropSuite->propSetString(props, kOfxImageEffectPluginRenderThreadSafety, 0, kind.threadSafety);
    gPropSuite->propSetInt(props, kOfxImageEffectPluginPropSingleInstance, 0, 0);
    // it fetches other frames than the one it renders; Natron also caches the output of such an effect
    gPropSuite->propSetInt(props, kOfxImageEffectPropTemporalClipAccess, 0, 1);
    if (kind.openGL) {
        // no concurrent OpenGL renders (the default of the OpenGL render suite): Natron keeps the effect's
        // attachment lock during the OpenGL render
        gPropSuite->propSetString(props, kOfxImageEffectPropOpenGLRenderSupported, 0, "true");
        gPropSuite->propSetString(props, kOfxImageEffectPropCPURenderSupported, 0, "true");
    }

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
    if (gParamSuite->paramDefine(paramSet, kOfxParamTypeString, kParamWaitFor, &props) != kOfxStatOK) {
        return kOfxStatFailed;
    }
    gPropSuite->propSetString(props, kOfxParamPropDefault, 0, "");

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

// Counts the threads running code of the render action of an Unsafe plug-in, while it exists (pause() around the
// host calls that may render upstream)
class UnsafeCodeRunning
{
public:
    explicit UnsafeCodeRunning(const PluginKind& kind)
        : _label( kind.isUnsafe() ? kind.label : "" )
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
            --gUnsafeCodeRunning[_label];
            _running = false;
        }
    }

    void resume()
    {
        if ( !_label.empty() && !_running ) {
            std::lock_guard<std::mutex> l(gMutex);
            if (++gUnsafeCodeRunning[_label] > 1) {
                natronTestIncrementCounter("RenderLock." + _label + ".concurrent");
            }
            _running = true;
        }
    }

private:
    const std::string _label; // empty if not Unsafe
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

// Fetches the image (CPU render) or the texture (OpenGL render) of Source at time; counts it in
// "RenderLock.<label>.fetched", and the kOfxStatErrMemory of clipLoadTexture in "RenderLock.<label>.textureErrMemory"
void
fetch(OfxImageEffectHandle effect,
      const PluginKind& kind,
      OfxTime time,
      bool openGLRender)
{
    OfxImageClipHandle source = 0;
    if (gEffectSuite->clipGetHandle(effect, kOfxImageEffectSimpleSourceClipName, &source, 0) != kOfxStatOK) {
        return;
    }
    const std::string label = kind.label;
    const OfxRectD* region = kind.openGL ? &kGLFetchRegion : 0;
    FetchingThread fetching;
    OfxPropertySetHandle image = 0;
    if (openGLRender) {
        const OfxStatus stat = gGLSuite->clipLoadTexture(source, time, 0, region, &image);
        if (stat == kOfxStatOK) {
            natronTestIncrementCounter("RenderLock." + label + ".fetched");
            gGLSuite->clipFreeTexture(image);
        } else if (stat == kOfxStatErrMemory) {
            natronTestIncrementCounter("RenderLock." + label + ".textureErrMemory");
        }
    } else if (gEffectSuite->clipGetImage(source, time, region, &image) == kOfxStatOK) {
        natronTestIncrementCounter("RenderLock." + label + ".fetched");
        gEffectSuite->clipReleaseImage(image);
    }
}

OfxStatus
render(OfxImageEffectHandle effect,
       OfxPropertySetHandle inArgs,
       const PluginKind& kind)
{
    const std::string label = kind.label;
    ActionInProgress inProgress(effect, label);
    UnsafeCodeRunning running(kind);
    OfxTime time = 0.;
    gPropSuite->propGetDouble(inArgs, kOfxPropTime, 0, &time);
    int openGLEnabled = 0;
    const bool openGLRender = kind.openGL &&
                              (gPropSuite->propGetInt(inArgs, kOfxImageEffectPropOpenGLEnabled, 0, &openGLEnabled) == kOfxStatOK) &&
                              openGLEnabled;
    if (openGLRender) {
        natronTestIncrementCounter("RenderLock." + label + ".glRenders");
    }
    if (time >= kFetchOffset) {
        natronTestIncrementCounter( "RenderLock." + label + ( FetchingThread::isCurrent() ? ".renderedByFetcher" : ".renderedElsewhere" ) );
    }
    OfxParamSetHandle paramSet = 0;
    OfxParamHandle fetchTimeParam = 0;
    OfxParamHandle waitForParam = 0;
    double fetchTime = -1.;
    char* waitForValue = 0;
    if ( (gEffectSuite->getParamSet(effect, &paramSet) != kOfxStatOK) ||
         (gParamSuite->paramGetHandle(paramSet, kParamFetchTime, &fetchTimeParam, 0) != kOfxStatOK) ||
         (gParamSuite->paramGetValueAtTime(fetchTimeParam, time, &fetchTime) != kOfxStatOK) ||
         (gParamSuite->paramGetHandle(paramSet, kParamWaitFor, &waitForParam, 0) != kOfxStatOK) ||
         (gParamSuite->paramGetValueAtTime(waitForParam, time, &waitForValue) != kOfxStatOK) ) {
        return kOfxStatFailed;
    }
    if (time != fetchTime) {
        return kOfxStatOK;
    }
    const std::string waitFor = (waitForValue && *waitForValue) ? waitForValue : kind.defaultWaitFor;
    if (waitFor != "-") {
        natronTestSetCounter("RenderLock." + label + ".inRender", 1);
        if ( !waitForCounter("RenderLock." + waitFor + ".inRender") ) {
            natronTestIncrementCounter("RenderLock." + label + ".waitTimeout");
        }
    }
    running.pause(); // Natron may run other renders of the plug-in while this one waits in the fetch
    fetch(effect, kind, time + kFetchOffset, openGLRender);
    running.resume();

    return kOfxStatOK;
}

OfxStatus
pluginMain(const char* action,
           OfxImageEffectHandle effect,
           OfxPropertySetHandle inArgs,
           OfxPropertySetHandle outArgs,
           const PluginKind& kind)
{
    if (std::strcmp(action, kOfxActionLoad) == 0) {
        if (!gHost) {
            return kOfxStatErrMissingHostFeature;
        }
        gPropSuite = (const OfxPropertySuiteV1*)gHost->fetchSuite(gHost->host, kOfxPropertySuite, 1);
        gEffectSuite = (const OfxImageEffectSuiteV1*)gHost->fetchSuite(gHost->host, kOfxImageEffectSuite, 1);
        gParamSuite = (const OfxParameterSuiteV1*)gHost->fetchSuite(gHost->host, kOfxParameterSuite, 1);
        gGLSuite = (const OfxImageEffectOpenGLRenderSuiteV1*)gHost->fetchSuite(gHost->host, kOfxOpenGLRenderSuite, 1);

        return (gPropSuite && gEffectSuite && gParamSuite && (gGLSuite || !kind.openGL) ) ? kOfxStatOK : kOfxStatErrMissingHostFeature;
    }
    if (std::strcmp(action, kOfxActionDescribe) == 0) {
        return describe(effect, kind);
    }
    if (std::strcmp(action, kOfxImageEffectActionDescribeInContext) == 0) {
        return describeInContext(effect);
    }
    if (std::strcmp(action, kOfxImageEffectActionGetClipPreferences) == 0) {
        // each frame is a different image
        gPropSuite->propSetInt(outArgs, kOfxImageEffectFrameVarying, 0, 1);

        return kOfxStatOK;
    }
    if ( kind.openGL && (std::strcmp(action, kOfxImageEffectActionGetRegionsOfInterest) == 0) ) {
        // nothing: Natron renders the input only when the action fetches it
        const OfxRectD empty = { 0., 0., 0., 0. };
        gPropSuite->propSetDoubleN(outArgs, "OfxImageClipPropRoI_" kOfxImageEffectSimpleSourceClipName, 4, &empty.x1);

        return kOfxStatOK;
    }
    if (std::strcmp(action, kOfxImageEffectActionRender) == 0) {
        return render(effect, inArgs, kind);
    }

    return kOfxStatReplyDefault;
}

OfxStatus
unsafeMain(const char* action,
           const void* handle,
           OfxPropertySetHandle inArgs,
           OfxPropertySetHandle outArgs)
{
    return pluginMain(action, (OfxImageEffectHandle)handle, inArgs, outArgs, kUnsafe);
}

OfxStatus
instanceSafeMain(const char* action,
                 const void* handle,
                 OfxPropertySetHandle inArgs,
                 OfxPropertySetHandle outArgs)
{
    return pluginMain(action, (OfxImageEffectHandle)handle, inArgs, outArgs, kInstanceSafe);
}

OfxStatus
unsafeGLMain(const char* action,
             const void* handle,
             OfxPropertySetHandle inArgs,
             OfxPropertySetHandle outArgs)
{
    return pluginMain(action, (OfxImageEffectHandle)handle, inArgs, outArgs, kUnsafeGL);
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
OfxPlugin gUnsafeGLPlugin = {
    kOfxImageEffectPluginApi, 1, "net.natron.test.RenderLockUnsafeGL", 1, 0, setHost, unsafeGLMain
};
NatronTestPluginRegistrar gUnsafeRegistrar(&gUnsafePlugin);
NatronTestPluginRegistrar gInstanceSafeRegistrar(&gInstanceSafePlugin);
NatronTestPluginRegistrar gUnsafeGLRegistrar(&gUnsafeGLPlugin);
} // anonymous namespace
