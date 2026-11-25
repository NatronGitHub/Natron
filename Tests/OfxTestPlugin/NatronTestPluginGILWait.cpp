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

// The plug-ins of Tests/OfxInstanceChangedGIL_Test.cpp (#1653): a kOfxActionInstanceChanged run by a
// Python script holds the Python GIL, and must not wait for threads that need it. Counters ("GILWait.*"):
// - net.natron.test.GILWaitSource: a generator (kOfxImageEffectRenderInstanceSafe, so that its renders
//   take its render lock) that reads its "value" parameter in render ("sourceRender"). If "holdRender" is
//   set when a render starts, that render clears it, sets "inRender" and waits (at most 20 s) for "go"
//   before it reads "value".
// - net.natron.test.GILWaitAnalysis: a filter whose push buttons do, in kOfxActionInstanceChanged:
//   - "fetch": fetch Source at the time of the action, as an analysis does ("fetched", "fetchFailed");
//   - "threads": read its own "value" parameter at the times 100 + threadIndex in 2 threads of the
//     multi-thread suite ("threadReads", "threadReadsFailed"; "threadReadsOnOtherThread" counts the reads on
//     another thread than the one that called multiThread, which may run them itself), and record
//     multiThreadNumCPUs ("numCPUs").

#include <chrono>
#include <cstring>
#include <thread>

#include <ofxCore.h>
#include <ofxImageEffect.h>
#include <ofxMultiThread.h>
#include <ofxParam.h>
#include <ofxProperty.h>

#include "NatronTestPluginRegistry.h"

extern "C" {
// NatronTestPlugin.cpp
OfxExport int NatronTestPluginGetCounter(const char* name);
}

namespace {
OfxHost* gHost = 0;
const OfxPropertySuiteV1* gPropSuite = 0;
const OfxImageEffectSuiteV1* gEffectSuite = 0;
const OfxParameterSuiteV1* gParamSuite = 0;
const OfxMultiThreadSuiteV1* gThreadSuite = 0;

OfxStatus
onLoad()
{
    if (!gHost) {
        return kOfxStatErrMissingHostFeature;
    }
    gPropSuite = (const OfxPropertySuiteV1*)gHost->fetchSuite(gHost->host, kOfxPropertySuite, 1);
    gEffectSuite = (const OfxImageEffectSuiteV1*)gHost->fetchSuite(gHost->host, kOfxImageEffectSuite, 1);
    gParamSuite = (const OfxParameterSuiteV1*)gHost->fetchSuite(gHost->host, kOfxParameterSuite, 1);
    gThreadSuite = (const OfxMultiThreadSuiteV1*)gHost->fetchSuite(gHost->host, kOfxMultiThreadSuite, 1);

    return (gPropSuite && gEffectSuite && gParamSuite && gThreadSuite) ? kOfxStatOK : kOfxStatErrMissingHostFeature;
}

OfxStatus
describe(OfxImageEffectHandle effect,
         const char* label,
         const char* context,
         const char* threadSafety)
{
    OfxPropertySetHandle props;
    OfxStatus stat = gEffectSuite->getPropertySet(effect, &props);

    if (stat != kOfxStatOK) {
        return stat;
    }
    gPropSuite->propSetString(props, kOfxPropLabel, 0, label);
    gPropSuite->propSetString(props, kOfxImageEffectPluginPropGrouping, 0, "Other/Test");
    gPropSuite->propSetString(props, kOfxImageEffectPropSupportedContexts, 0, context);
    gPropSuite->propSetString(props, kOfxImageEffectPropSupportedPixelDepths, 0, kOfxBitDepthFloat);
    gPropSuite->propSetString(props, kOfxImageEffectPluginRenderThreadSafety, 0, threadSafety);

    return kOfxStatOK;
}

OfxStatus
defineClip(OfxImageEffectHandle effect,
           const char* name)
{
    OfxPropertySetHandle props;
    OfxStatus stat = gEffectSuite->clipDefine(effect, name, &props);

    if (stat == kOfxStatOK) {
        gPropSuite->propSetString(props, kOfxImageEffectPropSupportedComponents, 0, kOfxImageComponentRGBA);
    }

    return stat;
}

OfxParamSetHandle
getParamSet(OfxImageEffectHandle effect)
{
    OfxParamSetHandle paramSet = 0;

    return (gEffectSuite->getParamSet(effect, &paramSet) == kOfxStatOK) ? paramSet : 0;
}

OfxStatus
defineParam(OfxImageEffectHandle effect,
            const char* type,
            const char* name)
{
    OfxParamSetHandle paramSet = getParamSet(effect);
    OfxPropertySetHandle props;

    return paramSet ? gParamSuite->paramDefine(paramSet, type, name, &props) : kOfxStatFailed;
}

OfxParamHandle
getParam(OfxImageEffectHandle effect,
         const char* name)
{
    OfxParamHandle param = 0;
    OfxParamSetHandle paramSet = getParamSet(effect);

    if ( !paramSet || (gParamSuite->paramGetHandle(paramSet, name, &param, 0) != kOfxStatOK) ) {
        return 0;
    }

    return param;
}

// read "value" at time: this evaluates its Python expression, which needs the GIL
bool
readValue(OfxImageEffectHandle effect,
          OfxTime time)
{
    OfxParamHandle param = getParam(effect, "value");
    double value = 0.;

    return param && (gParamSuite->paramGetValueAtTime(param, time, &value) == kOfxStatOK);
}

OfxStatus
commonAction(const char* action)
{
    if (std::strcmp(action, kOfxActionLoad) == 0) {
        return onLoad();
    }
    if (std::strcmp(action, kOfxImageEffectActionRender) == 0) {
        // the tests don't look at the images
        return kOfxStatOK;
    }

    return kOfxStatReplyDefault;
}

////////////////////////////////////////////////////////////////////////////////
// net.natron.test.GILWaitSource

OfxStatus
sourcePluginMain(const char* action,
                 const void* handle,
                 OfxPropertySetHandle inArgs,
                 OfxPropertySetHandle /*outArgs*/)
{
    OfxImageEffectHandle effect = (OfxImageEffectHandle)handle;

    if (std::strcmp(action, kOfxActionDescribe) == 0) {
        return describe(effect, "GILWaitSourceTest", kOfxImageEffectContextGenerator, kOfxImageEffectRenderInstanceSafe);
    }
    if (std::strcmp(action, kOfxImageEffectActionDescribeInContext) == 0) {
        OfxStatus stat = defineClip(effect, kOfxImageEffectOutputClipName);

        return (stat == kOfxStatOK) ? defineParam(effect, kOfxParamTypeDouble, "value") : stat;
    }
    if (std::strcmp(action, kOfxImageEffectActionRender) == 0) {
        if (NatronTestPluginGetCounter("GILWait.holdRender") != 0) {
            natronTestIncrementCounter("GILWait.holdRender", -NatronTestPluginGetCounter("GILWait.holdRender"));
            natronTestIncrementCounter("GILWait.inRender");
            for (int i = 0; i < 2000 && NatronTestPluginGetCounter("GILWait.go") == 0; ++i) {
                std::this_thread::sleep_for( std::chrono::milliseconds(10) );
            }
        }
        OfxTime time = 0.;
        gPropSuite->propGetDouble(inArgs, kOfxPropTime, 0, &time);
        readValue(effect, time);
        natronTestIncrementCounter("GILWait.sourceRender");

        return kOfxStatOK;
    }

    return commonAction(action);
}

////////////////////////////////////////////////////////////////////////////////
// net.natron.test.GILWaitAnalysis

struct ThreadArgs
{
    OfxImageEffectHandle effect;
    std::thread::id caller; // the thread that called multiThread
};

void
readValueThread(unsigned int threadIndex,
                unsigned int /*threadMax*/,
                void* customArg)
{
    const ThreadArgs* args = (const ThreadArgs*)customArg;

    natronTestIncrementCounter( readValue(args->effect, 100. + threadIndex) ? "GILWait.threadReads" : "GILWait.threadReadsFailed" );
    if ( std::this_thread::get_id() != args->caller ) {
        natronTestIncrementCounter("GILWait.threadReadsOnOtherThread");
    }
}

OfxStatus
analysisPluginMain(const char* action,
                   const void* handle,
                   OfxPropertySetHandle inArgs,
                   OfxPropertySetHandle /*outArgs*/)
{
    OfxImageEffectHandle effect = (OfxImageEffectHandle)handle;

    if (std::strcmp(action, kOfxActionDescribe) == 0) {
        return describe(effect, "GILWaitAnalysisTest", kOfxImageEffectContextFilter, kOfxImageEffectRenderFullySafe);
    }
    if (std::strcmp(action, kOfxImageEffectActionDescribeInContext) == 0) {
        const char* clips[] = { kOfxImageEffectSimpleSourceClipName, kOfxImageEffectOutputClipName };
        for (int i = 0; i < 2; ++i) {
            OfxStatus stat = defineClip(effect, clips[i]);
            if (stat != kOfxStatOK) {
                return stat;
            }
        }
        if ( (defineParam(effect, kOfxParamTypeDouble, "value") != kOfxStatOK) ||
             (defineParam(effect, kOfxParamTypePushButton, "fetch") != kOfxStatOK) ||
             (defineParam(effect, kOfxParamTypePushButton, "threads") != kOfxStatOK) ) {
            return kOfxStatFailed;
        }

        return kOfxStatOK;
    }
    if (std::strcmp(action, kOfxActionInstanceChanged) == 0) {
        const char* name = 0;
        gPropSuite->propGetString(inArgs, kOfxPropName, 0, &name);
        if ( name && (std::strcmp(name, "fetch") == 0) ) {
            OfxTime time = 0.;
            gPropSuite->propGetDouble(inArgs, kOfxPropTime, 0, &time);
            OfxImageClipHandle source = 0;
            OfxPropertySetHandle image = 0;
            if ( (gEffectSuite->clipGetHandle(effect, kOfxImageEffectSimpleSourceClipName, &source, 0) == kOfxStatOK) &&
                 (gEffectSuite->clipGetImage(source, time, 0, &image) == kOfxStatOK) ) {
                gEffectSuite->clipReleaseImage(image);
                natronTestIncrementCounter("GILWait.fetched");
            } else {
                natronTestIncrementCounter("GILWait.fetchFailed");
            }
        } else if ( name && (std::strcmp(name, "threads") == 0) ) {
            unsigned int nCPUs = 0;
            gThreadSuite->multiThreadNumCPUs(&nCPUs);
            natronTestIncrementCounter("GILWait.numCPUs", (int)nCPUs - NatronTestPluginGetCounter("GILWait.numCPUs"));
            ThreadArgs args = { effect, std::this_thread::get_id() };
            gThreadSuite->multiThread(readValueThread, 2, &args);
        }

        return kOfxStatOK;
    }

    return commonAction(action);
}

void
setHost(OfxHost* host)
{
    gHost = host;
}

OfxPlugin gSourcePlugin = {
    kOfxImageEffectPluginApi, 1, "net.natron.test.GILWaitSource", 1, 0, setHost, sourcePluginMain
};
OfxPlugin gAnalysisPlugin = {
    kOfxImageEffectPluginApi, 1, "net.natron.test.GILWaitAnalysis", 1, 0, setHost, analysisPluginMain
};
NatronTestPluginRegistrar gSourceRegistrar(&gSourcePlugin);
NatronTestPluginRegistrar gAnalysisRegistrar(&gAnalysisPlugin);
} // anonymous namespace
