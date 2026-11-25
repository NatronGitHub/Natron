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

// com.digitalfilmtools.NatronTestHostName (Tests/OfxHostProperties_Test.cpp, #1497): a filter with an overlay
// interact that reads the host name (kOfxPropName of the host) in its interact actions and in the thread
// functions of the multithread suite. Its ID starts with "com.digitalfilmtools.", a prefix to which Natron
// gives another host name (getHostNameProxy() in Engine/OfxHost.cpp), so that the tests can tell whether the
// host knows which plug-in reads the name. The ID borrows that prefix only to trigger the proxy: this is not
// a Digital Film Tools product. Counters ("HostName.*"):
// - createInstance.name.<host name>: the host name in kOfxActionCreateInstance, an effect action (if this
//   isn't the proxied name, the proxy table changed);
// - penDown.name.<host name>: the host name in kOfxInteractActionPenDown;
// - threaded.name.<host name> and sequential.name.<host name>: the host name in each thread function of the
//   multiThread() calls that kOfxInteractActionPenDown makes, with 2 threads and with 1 thread (which runs in
//   the calling thread);
// - threaded.spawned: the thread functions of the 2-thread call for which multiThreadIsSpawnedThread() is
//   true (also in the calling thread), threaded.otherThread: those that ran in another thread than the
//   calling one, and multiThread.numCPUs: multiThreadNumCPUs() at that call;
// - multiThread.failed: a multiThread() call that failed.

#include <cstring>
#include <string>
#include <thread>

#include <ofxCore.h>
#include <ofxImageEffect.h>
#include <ofxInteract.h>
#include <ofxMultiThread.h>
#include <ofxProperty.h>

#include "NatronTestPluginRegistry.h"

namespace {
OfxHost* gHost = 0;
const OfxPropertySuiteV1* gPropSuite = 0;
const OfxImageEffectSuiteV1* gEffectSuite = 0;
const OfxMultiThreadSuiteV1* gThreadSuite = 0;

const std::string kPrefix("HostName.");
std::thread::id gCallingThread; // the thread that calls multiThread()

void
recordHostName(const std::string& name)
{
    const char* hostName = 0;

    if ( (gPropSuite->propGetString(gHost->host, kOfxPropName, 0, &hostName) != kOfxStatOK) || !hostName ) {
        natronTestIncrementCounter(kPrefix + name + ".noName");

        return;
    }
    natronTestIncrementCounter(kPrefix + name + ".name." + hostName);
}

void
threadedFunction(unsigned int /*threadIndex*/,
                 unsigned int /*threadMax*/,
                 void* /*customArg*/)
{
    recordHostName("threaded");
    if ( gThreadSuite->multiThreadIsSpawnedThread() ) {
        natronTestIncrementCounter(kPrefix + "threaded.spawned");
    }
    if (std::this_thread::get_id() != gCallingThread) {
        natronTestIncrementCounter(kPrefix + "threaded.otherThread");
    }
}

void
sequentialFunction(unsigned int /*threadIndex*/,
                   unsigned int /*threadMax*/,
                   void* /*customArg*/)
{
    recordHostName("sequential");
}

OfxStatus
overlayMain(const char* action,
            const void* /*handle*/,
            OfxPropertySetHandle /*inArgs*/,
            OfxPropertySetHandle /*outArgs*/)
{
    if ( (std::strcmp(action, kOfxActionDescribe) == 0) || (std::strcmp(action, kOfxActionCreateInstance) == 0) ||
         (std::strcmp(action, kOfxActionDestroyInstance) == 0) ) {
        return kOfxStatOK;
    }
    if (std::strcmp(action, kOfxInteractActionPenDown) == 0) {
        recordHostName("penDown");
        gCallingThread = std::this_thread::get_id();
        unsigned int nCPUs = 0;
        if (gThreadSuite->multiThreadNumCPUs(&nCPUs) == kOfxStatOK) {
            natronTestIncrementCounter(kPrefix + "multiThread.numCPUs", (int)nCPUs);
        }
        if ( (gThreadSuite->multiThread(threadedFunction, 2, 0) != kOfxStatOK) ||
             (gThreadSuite->multiThread(sequentialFunction, 1, 0) != kOfxStatOK) ) {
            natronTestIncrementCounter(kPrefix + "multiThread.failed");
        }
    }

    return kOfxStatReplyDefault;
}

OfxStatus
hostNamePluginMain(const char* action,
                   const void* handle,
                   OfxPropertySetHandle /*inArgs*/,
                   OfxPropertySetHandle /*outArgs*/)
{
    OfxImageEffectHandle effect = (OfxImageEffectHandle)handle;

    if (std::strcmp(action, kOfxActionLoad) == 0) {
        if (!gHost) {
            return kOfxStatErrMissingHostFeature;
        }
        gPropSuite = (const OfxPropertySuiteV1*)gHost->fetchSuite(gHost->host, kOfxPropertySuite, 1);
        gEffectSuite = (const OfxImageEffectSuiteV1*)gHost->fetchSuite(gHost->host, kOfxImageEffectSuite, 1);
        gThreadSuite = (const OfxMultiThreadSuiteV1*)gHost->fetchSuite(gHost->host, kOfxMultiThreadSuite, 1);

        return (gPropSuite && gEffectSuite && gThreadSuite) ? kOfxStatOK : kOfxStatErrMissingHostFeature;
    }
    if (std::strcmp(action, kOfxActionCreateInstance) == 0) {
        recordHostName("createInstance");

        return kOfxStatOK;
    }
    if (std::strcmp(action, kOfxActionDescribe) == 0) {
        OfxPropertySetHandle props;
        OfxStatus stat = gEffectSuite->getPropertySet(effect, &props);
        if (stat != kOfxStatOK) {
            return stat;
        }
        gPropSuite->propSetString(props, kOfxPropLabel, 0, "HostNameTest");
        gPropSuite->propSetString(props, kOfxImageEffectPluginPropGrouping, 0, "Other/Test");
        gPropSuite->propSetString(props, kOfxImageEffectPropSupportedContexts, 0, kOfxImageEffectContextFilter);
        gPropSuite->propSetString(props, kOfxImageEffectPropSupportedPixelDepths, 0, kOfxBitDepthFloat);
        gPropSuite->propSetString(props, kOfxImageEffectPluginRenderThreadSafety, 0, kOfxImageEffectRenderFullySafe);
        gPropSuite->propSetPointer(props, kOfxImageEffectPluginPropOverlayInteractV1, 0, (void*)&overlayMain);

        return kOfxStatOK;
    }
    if (std::strcmp(action, kOfxImageEffectActionDescribeInContext) == 0) {
        const char* clips[] = { kOfxImageEffectSimpleSourceClipName, kOfxImageEffectOutputClipName };
        for (int i = 0; i < 2; ++i) {
            OfxPropertySetHandle props;
            OfxStatus stat = gEffectSuite->clipDefine(effect, clips[i], &props);
            if (stat != kOfxStatOK) {
                return stat;
            }
            gPropSuite->propSetString(props, kOfxImageEffectPropSupportedComponents, 0, kOfxImageComponentRGBA);
        }

        return kOfxStatOK;
    }

    return kOfxStatReplyDefault;
}

void
setHost(OfxHost* host)
{
    gHost = host;
}

OfxPlugin gPlugin = {
    kOfxImageEffectPluginApi, 1, "com.digitalfilmtools.NatronTestHostName", 1, 0, setHost, hostNamePluginMain
};
NatronTestPluginRegistrar gRegistrar(&gPlugin);
} // anonymous namespace
