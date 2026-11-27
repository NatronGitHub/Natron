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

// net.natron.test.RenderClone (Tests/OfxRenderClone_Test.cpp): an instance-safe generator
// (kOfxImageEffectRenderInstanceSafe), for which Natron makes a render clone of the instance when two
// renders run at the same time. The clones share the parameters of their main instance, so they must be
// destroyed before it (#1801). Counters ("RenderClone.*"):
// - created, destroyed, live: the instances (main instances and render clones);
// - destroyedBeforeLater: the instances destroyed while a later instance (e.g. one of their render
//   clones) was still alive;
// - inRender: the renders in progress. kOfxImageEffectActionRender blocks while the counter
//   RenderClone.blockRender is set, so that a test can keep a render going on the main instance;
// - instanceDataAtCreate: the instances whose kOfxPropInstanceData was not NULL in
//   kOfxActionCreateInstance (its default is NULL, ofxCore.h). Each instance then sets it to its own data.

#include <chrono>
#include <cstring>
#include <map>
#include <mutex>
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

std::mutex gMutex;
std::map<OfxImageEffectHandle, int> gLiveInstances; // the creation number of each live instance, guarded by gMutex
int gCreatedInstances = 0; // guarded by gMutex

OfxStatus
describe(OfxImageEffectHandle effect)
{
    OfxPropertySetHandle props;
    OfxStatus stat = gEffectSuite->getPropertySet(effect, &props);

    if (stat != kOfxStatOK) {
        return stat;
    }
    gPropSuite->propSetString(props, kOfxPropLabel, 0, "RenderCloneTest");
    gPropSuite->propSetString(props, kOfxImageEffectPluginPropGrouping, 0, "Other/Test");
    gPropSuite->propSetString(props, kOfxImageEffectPropSupportedContexts, 0, kOfxImageEffectContextGenerator);
    gPropSuite->propSetString(props, kOfxImageEffectPropSupportedPixelDepths, 0, kOfxBitDepthFloat);
    gPropSuite->propSetString(props, kOfxImageEffectPluginRenderThreadSafety, 0, kOfxImageEffectRenderInstanceSafe);
    gPropSuite->propSetInt(props, kOfxImageEffectPluginPropSingleInstance, 0, 0);

    return kOfxStatOK;
}

OfxStatus
describeInContext(OfxImageEffectHandle effect)
{
    OfxPropertySetHandle props;
    OfxStatus stat = gEffectSuite->clipDefine(effect, kOfxImageEffectOutputClipName, &props);

    if (stat != kOfxStatOK) {
        return stat;
    }
    gPropSuite->propSetString(props, kOfxImageEffectPropSupportedComponents, 0, kOfxImageComponentRGBA);

    // a parameter, which the render clones share with their main instance
    OfxParamSetHandle paramSet = 0;
    if ( (gEffectSuite->getParamSet(effect, &paramSet) != kOfxStatOK) ||
         (gParamSuite->paramDefine(paramSet, kOfxParamTypeDouble, "value", &props) != kOfxStatOK) ) {
        return kOfxStatFailed;
    }

    return kOfxStatOK;
}

void
onCreateInstance(OfxImageEffectHandle effect)
{
    // a render clone must not start with the instance data of its main instance
    OfxPropertySetHandle props = 0;
    void* instanceData = 0;
    if ( (gEffectSuite->getPropertySet(effect, &props) == kOfxStatOK) &&
         (gPropSuite->propGetPointer(props, kOfxPropInstanceData, 0, &instanceData) == kOfxStatOK) &&
         (instanceData != 0) ) {
        natronTestIncrementCounter("RenderClone.instanceDataAtCreate");
    }
    if (props) {
        gPropSuite->propSetPointer(props, kOfxPropInstanceData, 0, effect); // any pointer that is not NULL
    }
    {
        std::lock_guard<std::mutex> l(gMutex);
        gLiveInstances[effect] = ++gCreatedInstances;
    }
    natronTestIncrementCounter("RenderClone.created");
    natronTestIncrementCounter("RenderClone.live");
}

void
onDestroyInstance(OfxImageEffectHandle effect)
{
    bool beforeLater = false;
    {
        std::lock_guard<std::mutex> l(gMutex);
        std::map<OfxImageEffectHandle, int>::iterator found = gLiveInstances.find(effect);
        if ( found == gLiveInstances.end() ) {
            return;
        }
        for (std::map<OfxImageEffectHandle, int>::const_iterator it = gLiveInstances.begin(); it != gLiveInstances.end(); ++it) {
            if (it->second > found->second) {
                beforeLater = true;
            }
        }
        gLiveInstances.erase(found);
    }
    if (beforeLater) {
        natronTestIncrementCounter("RenderClone.destroyedBeforeLater");
    }
    natronTestIncrementCounter("RenderClone.destroyed");
    natronTestIncrementCounter("RenderClone.live", -1);
}

// Blocks while the counter RenderClone.blockRender is set.
void
render()
{
    natronTestIncrementCounter("RenderClone.inRender");
    while (natronTestGetCounter("RenderClone.blockRender") != 0) {
        std::this_thread::sleep_for( std::chrono::milliseconds(1) );
    }
    natronTestIncrementCounter("RenderClone.inRender", -1);
}

OfxStatus
renderClonePluginMain(const char* action,
                      const void* handle,
                      OfxPropertySetHandle /*inArgs*/,
                      OfxPropertySetHandle outArgs)
{
    OfxImageEffectHandle effect = (OfxImageEffectHandle)handle;

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
        return describe(effect);
    }
    if (std::strcmp(action, kOfxImageEffectActionDescribeInContext) == 0) {
        return describeInContext(effect);
    }
    if (std::strcmp(action, kOfxActionCreateInstance) == 0) {
        onCreateInstance(effect);

        return kOfxStatOK;
    }
    if (std::strcmp(action, kOfxActionDestroyInstance) == 0) {
        onDestroyInstance(effect);

        return kOfxStatOK;
    }
    if (std::strcmp(action, kOfxImageEffectActionGetClipPreferences) == 0) {
        // each frame is a different image, so that two frames render at the same time
        gPropSuite->propSetInt(outArgs, kOfxImageEffectFrameVarying, 0, 1);

        return kOfxStatOK;
    }
    if (std::strcmp(action, kOfxImageEffectActionRender) == 0) {
        render(); // the test doesn't look at the image

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
    kOfxImageEffectPluginApi, 1, "net.natron.test.RenderClone", 1, 0, setHost, renderClonePluginMain
};
NatronTestPluginRegistrar gRegistrar(&gPlugin);
} // anonymous namespace
