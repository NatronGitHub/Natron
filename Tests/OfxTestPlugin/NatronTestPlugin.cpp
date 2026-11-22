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

// A minimal OFX plug-in bundle for the unit tests (Tests/OfxTestPlugin.h), written against the OFX C
// API only, so that the tests can check how Natron, as an OFX host, calls the plug-in's actions. It is
// built in the build tree and never installed. Each plug-in records what happened in named counters,
// which the tests read with NatronTestPluginGetCounter().
//
// - net.natron.test.ParamInteract: a Double parameter with a parameter interact. Counts whether the
//   interact is destroyed before or after its effect instance ("An interact instance must always be
//   associated with an effect instance, so it gets created after the effect and destroyed before the
//   effect", ofxInteracts.rst).
// - net.natron.test.SingleInstance: sets kOfxImageEffectPluginPropSingleInstance, and counts its live
//   instances. Its kOfxImageEffectActionGetRegionOfDefinition blocks while the counter
//   SingleInstance.blockRoD is set.
// - net.natron.test.SyncPrivateData: keeps the number of changes of its "value" parameter in private
//   data, and only writes it to its "changeCount" parameter in kOfxActionSyncPrivateData.
// - net.natron.test.Params: parameters of every type, to test the parameter suite and the parameter
//   actions:
//   - pairs of parameters "<type>From" and "<type>To" of each type, for paramCopy;
//   - the custom parameters have an interpolation callback (kOfxParamPropCustomInterpCallbackV1), which
//     checks that its first argument is the parameter set of an instance;
//   - it checks that each kOfxActionInstanceChanged is bracketed by kOfxActionBeginInstanceChanged and
//     kOfxActionEndInstanceChanged with the same reason, and counts the errors ("Params.error.*");
//   - writing "chainSource" makes the plug-in write "chainTarget" in kOfxActionInstanceChanged;
//   - "preset" (kOfxParamPropCanUndo=0) is the "preset" example of the parameter suite documentation:
//     choosing a preset sets "presetA", "presetB" and "presetC" in a paramEditBegin/paramEditEnd block,
//     and sets "preset" back to 0;
//   - "uiOnly" has kOfxParamPropEvaluateOnChange=0, "noUndo" has kOfxParamPropCanUndo=0;
//   - "double2DRanged" has a range (kOfxParamPropMin/Max 0 to 1) but no display range
//     (Tests/OfxParamRange_Test.cpp).
// - net.natron.test.Generator: a generator that fills its output with 0.5, with the pixel aspect ratio of
//   its "par" parameter (it supports multiple clip PARs, so that it may set its output PAR, and "par" is a
//   clip preferences slave parameter). If its "fps" parameter (also a slave parameter) is positive, it sets
//   the output frame rate to it in getClipPreferences (kOfxImageEffectPropSetableFrameRate).
// - net.natron.test.MultiPAR: two inputs, Source and B, and kOfxImageEffectPropSupportsMultipleClipPARs.
//   It records the pixel aspect ratio of each input clip and image, times 1000, in render.
// - net.natron.test.ClipGetImage: asks for a small region of Source in getRegionsOfInterest; in render,
//   fetches that small region, then a larger one, and counts the images that don't cover the requested
//   region (clipped to the RoD) and the calls that returned the same handle twice.
// - net.natron.test.Sequential and net.natron.test.SequentialStatus: kOfxImageEffectInstancePropSequentialRender
//   is 1 and 0 on their descriptors. They count the renders with kOfxImageEffectPropSequentialRenderStatus 1
//   or 0, the renders with status 1 that are not in frame order (neither the frame of the previous render
//   with status 1 of the instance nor the next one), and the renders running at the same time. Writing the
//   "instanceSequential" parameter sets kOfxImageEffectInstancePropSequentialRender on the instance.
// - net.natron.test.TimeBlur: a temporal filter that fetches the frames t-1, t and t+1 of Source in render (and
//   asks for them in getFramesNeeded), and counts its renders.
// - net.natron.test.GPURenderFailed: supports OpenGL render, returns kOfxStatGPURenderFailed from the
//   OpenGL render, and renders on the CPU otherwise. net.natron.test.GPUOnlyRenderFailed does the same,
//   but sets kOfxImageEffectPropCPURenderSupported to "false".
// - net.natron.test.Question: the "ask" and "askFromThread" push buttons post a kOfxMessageQuestion from
//   kOfxActionInstanceChanged, on the calling thread or on a thread of the plug-in, and count the replies.
//
// The binary exports OfxSetHost (OFX 1.5), and counts its calls ("Binary.setHost"), whether it came
// before OfxGetNumberOfPlugins ("Binary.setHostBeforeGetNumberOfPlugins") and the kOfxPropAPIVersion
// of the host it gets ("Binary.setHostAPIVersion0", "Binary.setHostAPIVersion1"). ParamInteract
// calls clipGetHandle() in kOfxImageEffectActionDescribeInContext, where it is not valid, and records
// the status and whether the clip was set to NULL ("ParamInteract.describeClipGetHandle*").
//
// The plug-ins above count kOfxActionLoad and kOfxActionUnload ("<id>.load", "<id>.unload"). If the
// environment variable NATRON_TEST_PLUGIN_ACTION_LOG is set, they also append the lines "load <id>" and
// "unload <id> live=<live instances>" to the file it names, for the tests that check what a Natron
// process did before it exited (Tests/gui/run_renderer_exit_unload_test.py, #1557).

#include <chrono>
#include <climits>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <mutex>
#include <set>
#include <string>
#include <thread>
#include <vector>

#include <ofxCore.h>
#include <ofxImageEffect.h>
#include <ofxGPURender.h>
#include <ofxInteract.h>
#include <ofxMessage.h>
#include <ofxParam.h>
#include <ofxParametricParam.h>
#include <ofxProperty.h>

#include "NatronTestPluginRegistry.h"

#define kPluginGrouping "Other/Test"
#define kParamInteractPluginIdentifier "net.natron.test.ParamInteract"
#define kSingleInstancePluginIdentifier "net.natron.test.SingleInstance"
#define kSyncPrivateDataPluginIdentifier "net.natron.test.SyncPrivateData"
#define kParamsPluginIdentifier "net.natron.test.Params"
#define kGeneratorPluginIdentifier "net.natron.test.Generator"
#define kMultiPARPluginIdentifier "net.natron.test.MultiPAR"
#define kClipGetImagePluginIdentifier "net.natron.test.ClipGetImage"
#define kSequentialPluginIdentifier "net.natron.test.Sequential"
#define kSequentialStatusPluginIdentifier "net.natron.test.SequentialStatus"
#define kTimeBlurPluginIdentifier "net.natron.test.TimeBlur"
#define kGPURenderFailedPluginIdentifier "net.natron.test.GPURenderFailed"
#define kGPUOnlyRenderFailedPluginIdentifier "net.natron.test.GPUOnlyRenderFailed"
#define kQuestionPluginIdentifier "net.natron.test.Question"

namespace {
OfxHost* gHost = 0;
const OfxPropertySuiteV1* gPropSuite = 0;
const OfxImageEffectSuiteV1* gEffectSuite = 0;
const OfxParameterSuiteV1* gParamSuite = 0;
const OfxInteractSuiteV1* gInteractSuite = 0;
const OfxMessageSuiteV1* gMessageSuite = 0;

// everything below is protected by gMutex: the host may call the actions from several threads
std::mutex gMutex;
std::map<std::string, int> gCounters;
std::set<OfxImageEffectHandle> gLiveEffects;
std::map<OfxInteractHandle, OfxImageEffectHandle> gInteractEffects;
// net.natron.test.Params: the parameter sets of the live instances, and the reasons of the
// kOfxActionBeginInstanceChanged actions that have not been ended yet, by instance
std::set<OfxParamSetHandle> gParamsParamSets;
std::map<OfxImageEffectHandle, std::vector<std::string> > gParamsBrackets;

void
incrementCounter(const std::string& name,
                 int delta = 1)
{
    std::lock_guard<std::mutex> l(gMutex);
    gCounters[name] += delta;
}

OfxStatus
onLoad()
{
    if (!gHost) {
        return kOfxStatErrMissingHostFeature;
    }
    gPropSuite = (const OfxPropertySuiteV1*)gHost->fetchSuite(gHost->host, kOfxPropertySuite, 1);
    gEffectSuite = (const OfxImageEffectSuiteV1*)gHost->fetchSuite(gHost->host, kOfxImageEffectSuite, 1);
    gParamSuite = (const OfxParameterSuiteV1*)gHost->fetchSuite(gHost->host, kOfxParameterSuite, 1);
    gInteractSuite = (const OfxInteractSuiteV1*)gHost->fetchSuite(gHost->host, kOfxInteractSuite, 1);
    gMessageSuite = (const OfxMessageSuiteV1*)gHost->fetchSuite(gHost->host, kOfxMessageSuite, 1);
    if (!gPropSuite || !gEffectSuite || !gParamSuite || !gInteractSuite || !gMessageSuite) {
        return kOfxStatErrMissingHostFeature;
    }

    return kOfxStatOK;
}

OfxStatus
describe(OfxImageEffectHandle effect,
         const char* label,
         const char* threadSafety,
         bool singleInstance,
         const char* context = kOfxImageEffectContextFilter)
{
    OfxPropertySetHandle props;
    OfxStatus stat = gEffectSuite->getPropertySet(effect, &props);

    if (stat != kOfxStatOK) {
        return stat;
    }
    gPropSuite->propSetString(props, kOfxPropLabel, 0, label);
    gPropSuite->propSetString(props, kOfxImageEffectPluginPropGrouping, 0, kPluginGrouping);
    gPropSuite->propSetString(props, kOfxImageEffectPropSupportedContexts, 0, context);
    gPropSuite->propSetString(props, kOfxImageEffectPropSupportedPixelDepths, 0, kOfxBitDepthFloat);
    gPropSuite->propSetString(props, kOfxImageEffectPluginRenderThreadSafety, 0, threadSafety);
    gPropSuite->propSetInt(props, kOfxImageEffectPluginPropSingleInstance, 0, singleInstance ? 1 : 0);

    return kOfxStatOK;
}

OfxStatus
defineClips(OfxImageEffectHandle effect)
{
    OfxPropertySetHandle props;
    OfxStatus stat = gEffectSuite->clipDefine(effect, kOfxImageEffectSimpleSourceClipName, &props);

    if (stat != kOfxStatOK) {
        return stat;
    }
    gPropSuite->propSetString(props, kOfxImageEffectPropSupportedComponents, 0, kOfxImageComponentRGBA);
    stat = gEffectSuite->clipDefine(effect, kOfxImageEffectOutputClipName, &props);
    if (stat != kOfxStatOK) {
        return stat;
    }
    gPropSuite->propSetString(props, kOfxImageEffectPropSupportedComponents, 0, kOfxImageComponentRGBA);

    return kOfxStatOK;
}

OfxParamSetHandle
getParamSet(OfxImageEffectHandle effect)
{
    OfxParamSetHandle paramSet = 0;

    if (gEffectSuite->getParamSet(effect, &paramSet) != kOfxStatOK) {
        return 0;
    }

    return paramSet;
}

void
onCreateInstance(OfxImageEffectHandle effect,
                 const std::string& id)
{
    std::lock_guard<std::mutex> l(gMutex);
    gLiveEffects.insert(effect);
    int& live = gCounters[id + ".live"];
    ++live;
    int& maxLive = gCounters[id + ".maxLive"];
    if (live > maxLive) {
        maxLive = live;
    }
    ++gCounters[id + ".created"];
}

void
onDestroyInstance(OfxImageEffectHandle effect,
                  const std::string& id)
{
    std::lock_guard<std::mutex> l(gMutex);
    gLiveEffects.erase(effect);
    --gCounters[id + ".live"];
    ++gCounters[id + ".destroyed"];
}

// kOfxActionLoad and kOfxActionUnload of the plug-in id: see the comment at the top
void
logLoadAction(const char* action,
              const std::string& id)
{
    std::lock_guard<std::mutex> l(gMutex);
    const bool load = std::strcmp(action, kOfxActionLoad) == 0;
    ++gCounters[id + (load ? ".load" : ".unload")];
    const char* logPath = std::getenv("NATRON_TEST_PLUGIN_ACTION_LOG");
    if ( !logPath || !*logPath ) {
        return;
    }
    if ( std::FILE* f = std::fopen(logPath, "a") ) {
        if (load) {
            std::fprintf(f, "load %s\n", id.c_str());
        } else {
            std::fprintf(f, "unload %s live=%d\n", id.c_str(), gCounters[id + ".live"]);
        }
        std::fclose(f);
    }
}

// Main entry of the actions common to all the plug-ins, kOfxStatReplyDefault for the others.
OfxStatus
commonAction(const char* action,
             OfxImageEffectHandle effect,
             const std::string& id)
{
    if (std::strcmp(action, kOfxActionLoad) == 0) {
        const OfxStatus stat = onLoad();
        if (stat == kOfxStatOK) {
            logLoadAction(action, id);
        }

        return stat;
    }
    if (std::strcmp(action, kOfxActionUnload) == 0) {
        logLoadAction(action, id);

        return kOfxStatReplyDefault;
    }
    if (std::strcmp(action, kOfxActionCreateInstance) == 0) {
        onCreateInstance(effect, id);

        return kOfxStatOK;
    }
    if (std::strcmp(action, kOfxActionDestroyInstance) == 0) {
        onDestroyInstance(effect, id);

        return kOfxStatOK;
    }
    if (std::strcmp(action, kOfxImageEffectActionRender) == 0) {
        // the tests don't look at the images
        incrementCounter(id + ".render");

        return kOfxStatOK;
    }

    return kOfxStatReplyDefault;
}

////////////////////////////////////////////////////////////////////////////////
// net.natron.test.ParamInteract

OfxStatus
paramInteractMain(const char* action,
                  const void* handle,
                  OfxPropertySetHandle /*inArgs*/,
                  OfxPropertySetHandle /*outArgs*/)
{
    OfxInteractHandle interact = (OfxInteractHandle)handle;

    if (std::strcmp(action, kOfxActionDescribe) == 0) {
        return kOfxStatOK;
    }
    if (std::strcmp(action, kOfxActionCreateInstance) == 0) {
        OfxPropertySetHandle props;
        if (gInteractSuite->interactGetPropertySet(interact, &props) != kOfxStatOK) {
            return kOfxStatFailed;
        }
        void* effect = 0;
        gPropSuite->propGetPointer(props, kOfxPropEffectInstance, 0, &effect);
        std::lock_guard<std::mutex> l(gMutex);
        gInteractEffects[interact] = (OfxImageEffectHandle)effect;
        ++gCounters[gLiveEffects.count( (OfxImageEffectHandle)effect ) ? "ParamInteract.interactCreatedAfterEffect" :
                    "ParamInteract.interactCreatedBeforeEffect"];

        return kOfxStatOK;
    }
    if (std::strcmp(action, kOfxActionDestroyInstance) == 0) {
        // Don't touch kOfxPropEffectInstance here: if the effect is gone, it points to freed memory.
        std::lock_guard<std::mutex> l(gMutex);
        std::map<OfxInteractHandle, OfxImageEffectHandle>::iterator found = gInteractEffects.find(interact);
        if ( found == gInteractEffects.end() ) {
            return kOfxStatFailed;
        }
        ++gCounters[gLiveEffects.count(found->second) ? "ParamInteract.interactDestroyedBeforeEffect" :
                    "ParamInteract.interactDestroyedAfterEffect"];
        gInteractEffects.erase(found);

        return kOfxStatOK;
    }

    return kOfxStatReplyDefault;
}

OfxStatus
paramInteractPluginMain(const char* action,
                        const void* handle,
                        OfxPropertySetHandle /*inArgs*/,
                        OfxPropertySetHandle /*outArgs*/)
{
    OfxImageEffectHandle effect = (OfxImageEffectHandle)handle;

    if (std::strcmp(action, kOfxActionDescribe) == 0) {
        return describe(effect, "ParamInteractTest", kOfxImageEffectRenderFullySafe, false);
    }
    if (std::strcmp(action, kOfxImageEffectActionDescribeInContext) == 0) {
        OfxStatus stat = defineClips(effect);
        if (stat != kOfxStatOK) {
            return stat;
        }
        {
            // "Not a valid call in any of the describe actions": the host must return
            // kOfxStatErrBadHandle and set the clip to NULL (#1366)
            OfxImageClipHandle clip = (OfxImageClipHandle)&stat; // not NULL
            OfxStatus clipStat = gEffectSuite->clipGetHandle(effect, kOfxImageEffectSimpleSourceClipName, &clip, 0);
            incrementCounter(clipStat == kOfxStatErrBadHandle ? "ParamInteract.describeClipGetHandleBadHandle" :
                             "ParamInteract.describeClipGetHandleOtherStatus");
            incrementCounter(clip == 0 ? "ParamInteract.describeClipGetHandleNull" :
                             "ParamInteract.describeClipGetHandleNotNull");
        }
        OfxParamSetHandle paramSet = getParamSet(effect);
        OfxPropertySetHandle props;
        if ( !paramSet || (gParamSuite->paramDefine(paramSet, kOfxParamTypeDouble, "value", &props) != kOfxStatOK) ) {
            return kOfxStatFailed;
        }
        gPropSuite->propSetPointer(props, kOfxParamPropInteractV1, 0, (void*)&paramInteractMain);

        return kOfxStatOK;
    }

    return commonAction(action, effect, "ParamInteract");
}

////////////////////////////////////////////////////////////////////////////////
// net.natron.test.SingleInstance

OfxStatus
singleInstancePluginMain(const char* action,
                         const void* handle,
                         OfxPropertySetHandle /*inArgs*/,
                         OfxPropertySetHandle /*outArgs*/)
{
    OfxImageEffectHandle effect = (OfxImageEffectHandle)handle;

    if (std::strcmp(action, kOfxActionDescribe) == 0) {
        // InstanceSafe: the host would otherwise make extra instances to render in parallel
        return describe(effect, "SingleInstanceTest", kOfxImageEffectRenderInstanceSafe, true);
    }
    if (std::strcmp(action, kOfxImageEffectActionDescribeInContext) == 0) {
        return defineClips(effect);
    }
    if (std::strcmp(action, kOfxImageEffectActionGetRegionOfDefinition) == 0) {
        // While the counter SingleInstance.blockRoD is set, the action blocks (SingleInstance.inRoD
        // counts the threads in it): a test holds another thread inside the plug-in.
        bool block;
        {
            std::lock_guard<std::mutex> l(gMutex);
            block = gCounters["SingleInstance.blockRoD"] != 0;
            if (block) {
                ++gCounters["SingleInstance.inRoD"];
            }
        }
        while (block) {
            std::this_thread::sleep_for( std::chrono::milliseconds(1) );
            std::lock_guard<std::mutex> l(gMutex);
            block = gCounters["SingleInstance.blockRoD"] != 0;
            if (!block) {
                --gCounters["SingleInstance.inRoD"];
            }
        }

        return kOfxStatReplyDefault;
    }

    return commonAction(action, effect, "SingleInstance");
}

////////////////////////////////////////////////////////////////////////////////
// net.natron.test.SyncPrivateData

int*
getChangeCount(OfxImageEffectHandle effect)
{
    OfxPropertySetHandle props;
    void* data = 0;

    if ( (gEffectSuite->getPropertySet(effect, &props) != kOfxStatOK) ||
         (gPropSuite->propGetPointer(props, kOfxPropInstanceData, 0, &data) != kOfxStatOK) ) {
        return 0;
    }

    return (int*)data;
}

OfxStatus
syncPrivateDataPluginMain(const char* action,
                          const void* handle,
                          OfxPropertySetHandle inArgs,
                          OfxPropertySetHandle /*outArgs*/)
{
    OfxImageEffectHandle effect = (OfxImageEffectHandle)handle;

    if (std::strcmp(action, kOfxActionDescribe) == 0) {
        return describe(effect, "SyncPrivateDataTest", kOfxImageEffectRenderFullySafe, false);
    }
    if (std::strcmp(action, kOfxImageEffectActionDescribeInContext) == 0) {
        OfxStatus stat = defineClips(effect);
        if (stat != kOfxStatOK) {
            return stat;
        }
        OfxParamSetHandle paramSet = getParamSet(effect);
        OfxPropertySetHandle props;
        if ( !paramSet ||
             (gParamSuite->paramDefine(paramSet, kOfxParamTypeInteger, "value", &props) != kOfxStatOK) ||
             (gParamSuite->paramDefine(paramSet, kOfxParamTypeInteger, "changeCount", &props) != kOfxStatOK) ) {
            return kOfxStatFailed;
        }

        return kOfxStatOK;
    }
    if (std::strcmp(action, kOfxActionCreateInstance) == 0) {
        OfxPropertySetHandle props;
        if (gEffectSuite->getPropertySet(effect, &props) != kOfxStatOK) {
            return kOfxStatFailed;
        }
        // the private data starts from the saved value
        int saved = 0;
        OfxParamHandle param;
        OfxParamSetHandle paramSet = getParamSet(effect);
        if ( paramSet && (gParamSuite->paramGetHandle(paramSet, "changeCount", &param, 0) == kOfxStatOK) ) {
            gParamSuite->paramGetValue(param, &saved);
        }
        gPropSuite->propSetPointer(props, kOfxPropInstanceData, 0, new int(saved));
    } else if (std::strcmp(action, kOfxActionDestroyInstance) == 0) {
        delete getChangeCount(effect);
    } else if (std::strcmp(action, kOfxActionInstanceChanged) == 0) {
        const char* name = 0;
        int* changeCount = getChangeCount(effect);
        if ( changeCount && inArgs && (gPropSuite->propGetString(inArgs, kOfxPropName, 0, &name) == kOfxStatOK) &&
             name && (std::strcmp(name, "value") == 0) ) {
            ++*changeCount;
        }

        return kOfxStatOK;
    } else if (std::strcmp(action, kOfxActionSyncPrivateData) == 0) {
        incrementCounter("SyncPrivateData.sync");
        int* changeCount = getChangeCount(effect);
        OfxParamHandle param;
        OfxParamSetHandle paramSet = getParamSet(effect);
        if ( !changeCount || !paramSet || (gParamSuite->paramGetHandle(paramSet, "changeCount", &param, 0) != kOfxStatOK) ) {
            return kOfxStatFailed;
        }
        gParamSuite->paramSetValue(param, *changeCount);

        return kOfxStatOK;
    }

    return commonAction(action, effect, "SyncPrivateData");
}

////////////////////////////////////////////////////////////////////////////////
// net.natron.test.Params

// the parameter types of the "<type>From"/"<type>To" pairs
struct ParamsType
{
    const char* prefix;
    const char* type;
};

const ParamsType gParamsTypes[] = {
    { "int", kOfxParamTypeInteger },
    { "double", kOfxParamTypeDouble },
    { "bool", kOfxParamTypeBoolean },
    { "choice", kOfxParamTypeChoice },
    { "rgb", kOfxParamTypeRGB },
    { "rgba", kOfxParamTypeRGBA },
    { "double2D", kOfxParamTypeDouble2D },
    { "int2D", kOfxParamTypeInteger2D },
    { "double3D", kOfxParamTypeDouble3D },
    { "int3D", kOfxParamTypeInteger3D },
    { "string", kOfxParamTypeString },
    { "custom", kOfxParamTypeCustom },
    { "parametric", kOfxParamTypeParametric },
};

// kOfxParamPropCustomInterpCallbackV1 of the custom parameters: "<lower>><upper>@<amount>", or an error
// if the first argument is not the parameter set of an instance ("the plugin instance that this
// parameter occurs in"), or kOfxStatFailed if the lower keyframe is "fail".
OfxStatus
paramsCustomInterp(OfxParamSetHandle instance,
                   OfxPropertySetHandle inArgs,
                   OfxPropertySetHandle outArgs)
{
    {
        std::lock_guard<std::mutex> l(gMutex);
        if ( !gParamsParamSets.count(instance) ) {
            ++gCounters["Params.interp.badHandle"];

            return kOfxStatErrBadHandle;
        }
    }
    const char* lower = 0;
    const char* upper = 0;
    double amount = 0.;
    if ( (gPropSuite->propGetString(inArgs, kOfxParamPropCustomValue, 0, &lower) != kOfxStatOK) ||
         (gPropSuite->propGetString(inArgs, kOfxParamPropCustomValue, 1, &upper) != kOfxStatOK) ||
         (gPropSuite->propGetDouble(inArgs, kOfxParamPropInterpolationAmount, 0, &amount) != kOfxStatOK) ) {
        incrementCounter("Params.interp.badArgs");

        return kOfxStatFailed;
    }
    if (std::strcmp(lower, "fail") == 0) {
        incrementCounter("Params.interp.failed");

        return kOfxStatFailed;
    }
    std::string result = std::string(lower) + '>' + upper + '@';
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%.2f", amount);
    result += buf;
    gPropSuite->propSetString(outArgs, kOfxParamPropCustomValue, 0, result.c_str());
    incrementCounter("Params.interp.ok");

    return kOfxStatOK;
}

OfxStatus
paramsDefine(OfxParamSetHandle paramSet,
             const char* type,
             const std::string& name,
             OfxPropertySetHandle* props)
{
    OfxStatus stat = gParamSuite->paramDefine(paramSet, type, name.c_str(), props);

    if (stat != kOfxStatOK) {
        return stat;
    }
    if (std::strcmp(type, kOfxParamTypeChoice) == 0) {
        gPropSuite->propSetString(*props, kOfxParamPropChoiceOption, 0, "zero");
        gPropSuite->propSetString(*props, kOfxParamPropChoiceOption, 1, "one");
        gPropSuite->propSetString(*props, kOfxParamPropChoiceOption, 2, "two");
    } else if ( (std::strcmp(type, kOfxParamTypeString) == 0) || (std::strcmp(type, kOfxParamTypeCustom) == 0) ) {
        gPropSuite->propSetInt(*props, kOfxParamPropAnimates, 0, 1);
        if (std::strcmp(type, kOfxParamTypeCustom) == 0) {
            gPropSuite->propSetPointer(*props, kOfxParamPropCustomInterpCallbackV1, 0, (void*)&paramsCustomInterp);
        }
    } else if (std::strcmp(type, kOfxParamTypeParametric) == 0) {
        gPropSuite->propSetInt(*props, kOfxParamPropParametricDimension, 0, 1);
        gPropSuite->propSetDouble(*props, kOfxParamPropParametricRange, 0, 0.);
        gPropSuite->propSetDouble(*props, kOfxParamPropParametricRange, 1, 1.);
    }

    return kOfxStatOK;
}

OfxStatus
paramsDescribeInContext(OfxImageEffectHandle effect)
{
    OfxStatus stat = defineClips(effect);

    if (stat != kOfxStatOK) {
        return stat;
    }
    OfxParamSetHandle paramSet = getParamSet(effect);
    if (!paramSet) {
        return kOfxStatFailed;
    }
    OfxPropertySetHandle props;
    for (std::size_t i = 0; i < sizeof(gParamsTypes) / sizeof(gParamsTypes[0]); ++i) {
        if ( (paramsDefine(paramSet, gParamsTypes[i].type, std::string(gParamsTypes[i].prefix) + "From", &props) != kOfxStatOK) ||
             (paramsDefine(paramSet, gParamsTypes[i].type, std::string(gParamsTypes[i].prefix) + "To", &props) != kOfxStatOK) ) {
            return kOfxStatFailed;
        }
    }
    if ( (paramsDefine(paramSet, kOfxParamTypeInteger, "chainSource", &props) != kOfxStatOK) ||
         (paramsDefine(paramSet, kOfxParamTypeInteger, "chainTarget", &props) != kOfxStatOK) ) {
        return kOfxStatFailed;
    }
    if (paramsDefine(paramSet, kOfxParamTypeChoice, "preset", &props) != kOfxStatOK) {
        return kOfxStatFailed;
    }
    gPropSuite->propSetInt(props, kOfxParamPropCanUndo, 0, 0);
    if ( (paramsDefine(paramSet, kOfxParamTypeDouble, "presetA", &props) != kOfxStatOK) ||
         (paramsDefine(paramSet, kOfxParamTypeDouble, "presetB", &props) != kOfxStatOK) ||
         (paramsDefine(paramSet, kOfxParamTypeDouble, "presetC", &props) != kOfxStatOK) ) {
        return kOfxStatFailed;
    }
    // a group (Tests/OfxSuiteStatus_Test.cpp: kOfxParamPropGroupOpen written on the instance)
    if (paramsDefine(paramSet, kOfxParamTypeGroup, "group", &props) != kOfxStatOK) {
        return kOfxStatFailed;
    }
    if (paramsDefine(paramSet, kOfxParamTypeBoolean, "uiOnly", &props) != kOfxStatOK) {
        return kOfxStatFailed;
    }
    gPropSuite->propSetInt(props, kOfxParamPropEvaluateOnChange, 0, 0);
    if (paramsDefine(paramSet, kOfxParamTypeDouble, "noUndo", &props) != kOfxStatOK) {
        return kOfxStatFailed;
    }
    gPropSuite->propSetInt(props, kOfxParamPropCanUndo, 0, 0);
    // a range without a display range: the display range keeps its default, -DBL_MAX to DBL_MAX
    if (paramsDefine(paramSet, kOfxParamTypeDouble2D, "double2DRanged", &props) != kOfxStatOK) {
        return kOfxStatFailed;
    }
    for (int i = 0; i < 2; ++i) {
        gPropSuite->propSetDouble(props, kOfxParamPropMin, i, 0.);
        gPropSuite->propSetDouble(props, kOfxParamPropMax, i, 1.);
    }

    return kOfxStatOK;
}

// kOfxActionBeginInstanceChanged and kOfxActionEndInstanceChanged
void
paramsBracket(OfxImageEffectHandle effect,
              OfxPropertySetHandle inArgs,
              bool begin)
{
    const char* reason = 0;

    if ( !inArgs || (gPropSuite->propGetString(inArgs, kOfxPropChangeReason, 0, &reason) != kOfxStatOK) || !reason ) {
        incrementCounter("Params.error.noReason");

        return;
    }
    std::lock_guard<std::mutex> l(gMutex);
    std::vector<std::string>& brackets = gParamsBrackets[effect];
    if (begin) {
        brackets.push_back(reason);
        ++gCounters["Params.begin"];
        ++gCounters[std::string("Params.begin.") + reason];
        ++gCounters["Params.openBrackets"];
        int& maxDepth = gCounters["Params.maxDepth"];
        if ( (int)brackets.size() > maxDepth ) {
            maxDepth = (int)brackets.size();
        }

        return;
    }
    ++gCounters["Params.end"];
    ++gCounters[std::string("Params.end.") + reason];
    if ( brackets.empty() ) {
        ++gCounters["Params.error.unmatchedEnd"];

        return;
    }
    if (brackets.back() != reason) {
        ++gCounters["Params.error.endReasonMismatch"];
    }
    brackets.pop_back();
    --gCounters["Params.openBrackets"];
}

OfxStatus
paramsSetInt(OfxParamSetHandle paramSet,
             const char* name,
             int value)
{
    OfxParamHandle param;
    OfxStatus stat = gParamSuite->paramGetHandle(paramSet, name, &param, 0);

    return stat == kOfxStatOK ? gParamSuite->paramSetValue(param, value) : stat;
}

OfxStatus
paramsSetDouble(OfxParamSetHandle paramSet,
                const char* name,
                double value)
{
    OfxParamHandle param;
    OfxStatus stat = gParamSuite->paramGetHandle(paramSet, name, &param, 0);

    return stat == kOfxStatOK ? gParamSuite->paramSetValue(param, value) : stat;
}

OfxStatus
paramsInstanceChanged(OfxImageEffectHandle effect,
                      OfxPropertySetHandle inArgs)
{
    const char* type = 0;
    const char* name = 0;
    const char* reason = 0;

    if ( !inArgs ||
         (gPropSuite->propGetString(inArgs, kOfxPropType, 0, &type) != kOfxStatOK) ||
         (gPropSuite->propGetString(inArgs, kOfxPropName, 0, &name) != kOfxStatOK) ||
         (gPropSuite->propGetString(inArgs, kOfxPropChangeReason, 0, &reason) != kOfxStatOK) ) {
        incrementCounter("Params.error.badInstanceChangedArgs");

        return kOfxStatFailed;
    }
    {
        std::lock_guard<std::mutex> l(gMutex);
        ++gCounters["Params.instanceChanged"];
        ++gCounters[std::string("Params.instanceChanged.") + name];
        const std::vector<std::string>& brackets = gParamsBrackets[effect];
        if ( brackets.empty() ) {
            ++gCounters["Params.error.instanceChangedOutsideBracket"];
        } else if (brackets.back() != reason) {
            ++gCounters["Params.error.instanceChangedReasonMismatch"];
        }
    }
    if (std::strcmp(type, kOfxTypeParameter) != 0) {
        return kOfxStatOK;
    }
    OfxParamSetHandle paramSet = getParamSet(effect);
    if (!paramSet) {
        return kOfxStatFailed;
    }
    if (std::strcmp(name, "chainSource") == 0) {
        OfxParamHandle param;
        int value = 0;
        if ( (gParamSuite->paramGetHandle(paramSet, name, &param, 0) != kOfxStatOK) ||
             (gParamSuite->paramGetValue(param, &value) != kOfxStatOK) ) {
            return kOfxStatFailed;
        }

        return paramsSetInt(paramSet, "chainTarget", value + 1);
    }
    if (std::strcmp(name, "preset") == 0) {
        OfxParamHandle param;
        int value = 0;
        if ( (gParamSuite->paramGetHandle(paramSet, name, &param, 0) != kOfxStatOK) ||
             (gParamSuite->paramGetValue(param, &value) != kOfxStatOK) ) {
            return kOfxStatFailed;
        }
        if (value == 0) {
            return kOfxStatOK;
        }
        // the "preset" example of the documentation of paramEditBegin: the first write of the block goes
        // to a parameter that is not undoable
        incrementCounter("Params.presetApplied");
        gParamSuite->paramEditBegin(paramSet, "Apply preset");
        paramsSetInt(paramSet, "preset", 0);
        paramsSetDouble(paramSet, "presetA", value * 1.);
        paramsSetDouble(paramSet, "presetB", value * 2.);
        paramsSetDouble(paramSet, "presetC", value * 3.);
        gParamSuite->paramEditEnd(paramSet);
    }

    return kOfxStatOK;
}

OfxStatus
paramsPluginMain(const char* action,
                 const void* handle,
                 OfxPropertySetHandle inArgs,
                 OfxPropertySetHandle /*outArgs*/)
{
    OfxImageEffectHandle effect = (OfxImageEffectHandle)handle;

    if (std::strcmp(action, kOfxActionDescribe) == 0) {
        return describe(effect, "ParamsTest", kOfxImageEffectRenderFullySafe, false);
    }
    if (std::strcmp(action, kOfxImageEffectActionDescribeInContext) == 0) {
        return paramsDescribeInContext(effect);
    }
    if (std::strcmp(action, kOfxActionCreateInstance) == 0) {
        OfxParamSetHandle paramSet = getParamSet(effect);
        std::lock_guard<std::mutex> l(gMutex);
        gParamsParamSets.insert(paramSet);
    } else if (std::strcmp(action, kOfxActionDestroyInstance) == 0) {
        OfxParamSetHandle paramSet = getParamSet(effect);
        std::lock_guard<std::mutex> l(gMutex);
        gParamsParamSets.erase(paramSet);
        if ( !gParamsBrackets[effect].empty() ) {
            gCounters["Params.error.destroyedInBracket"] += (int)gParamsBrackets[effect].size();
            gCounters["Params.openBrackets"] -= (int)gParamsBrackets[effect].size();
        }
        gParamsBrackets.erase(effect);
    } else if (std::strcmp(action, kOfxActionBeginInstanceChanged) == 0) {
        paramsBracket(effect, inArgs, true);

        return kOfxStatOK;
    } else if (std::strcmp(action, kOfxActionEndInstanceChanged) == 0) {
        paramsBracket(effect, inArgs, false);

        return kOfxStatOK;
    } else if (std::strcmp(action, kOfxActionInstanceChanged) == 0) {
        return paramsInstanceChanged(effect, inArgs);
    }

    return commonAction(action, effect, "Params");
}

////////////////////////////////////////////////////////////////////////////////
// Helpers for the plug-ins that render

void
setCounter(const std::string& name,
           int value)
{
    std::lock_guard<std::mutex> l(gMutex);
    gCounters[name] = value;
}

OfxImageClipHandle
getClip(OfxImageEffectHandle effect,
        const char* name)
{
    OfxImageClipHandle clip = 0;

    if (gEffectSuite->clipGetHandle(effect, name, &clip, 0) != kOfxStatOK) {
        return 0;
    }

    return clip;
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

// The name of the parameter that changed, in kOfxActionInstanceChanged, or an empty string.
std::string
changedParamName(OfxPropertySetHandle inArgs)
{
    const char* type = 0;
    const char* name = 0;

    if ( !inArgs || (gPropSuite->propGetString(inArgs, kOfxPropType, 0, &type) != kOfxStatOK) ||
         !type || (std::strcmp(type, kOfxTypeParameter) != 0) ||
         (gPropSuite->propGetString(inArgs, kOfxPropName, 0, &name) != kOfxStatOK) || !name ) {
        return std::string();
    }

    return name;
}

bool
contains(const OfxRectI& outer,
         const OfxRectI& inner)
{
    return outer.x1 <= inner.x1 && outer.y1 <= inner.y1 && inner.x2 <= outer.x2 && inner.y2 <= outer.y2;
}

// Fill the render window of the output image (RGBA float, the only format the plug-ins support) with
// value, in a CPU render.
OfxStatus
fillOutput(OfxImageEffectHandle effect,
           OfxPropertySetHandle inArgs,
           float value)
{
    OfxTime time = 0.;
    OfxRectI renderWindow;
    OfxImageClipHandle output = getClip(effect, kOfxImageEffectOutputClipName);
    OfxPropertySetHandle image = 0;

    if ( !output || (gPropSuite->propGetDouble(inArgs, kOfxPropTime, 0, &time) != kOfxStatOK) ||
         (gPropSuite->propGetIntN(inArgs, kOfxImageEffectPropRenderWindow, 4, &renderWindow.x1) != kOfxStatOK) ||
         (gEffectSuite->clipGetImage(output, time, 0, &image) != kOfxStatOK) ) {
        return kOfxStatFailed;
    }
    void* data = 0;
    OfxRectI bounds;
    int rowBytes = 0;
    OfxStatus stat = kOfxStatOK;
    if ( (gPropSuite->propGetPointer(image, kOfxImagePropData, 0, &data) != kOfxStatOK) ||
         (gPropSuite->propGetIntN(image, kOfxImagePropBounds, 4, &bounds.x1) != kOfxStatOK) ||
         (gPropSuite->propGetInt(image, kOfxImagePropRowBytes, 0, &rowBytes) != kOfxStatOK) || !data ) {
        stat = kOfxStatFailed;
    } else {
        const int x1 = renderWindow.x1 > bounds.x1 ? renderWindow.x1 : bounds.x1;
        const int x2 = renderWindow.x2 < bounds.x2 ? renderWindow.x2 : bounds.x2;
        const int y1 = renderWindow.y1 > bounds.y1 ? renderWindow.y1 : bounds.y1;
        const int y2 = renderWindow.y2 < bounds.y2 ? renderWindow.y2 : bounds.y2;
        for (int y = y1; y < y2; ++y) {
            float* pix = (float*)( (char*)data + (std::ptrdiff_t)(y - bounds.y1) * rowBytes ) + 4 * (x1 - bounds.x1);
            for (int x = x1; x < x2; ++x, pix += 4) {
                pix[0] = pix[1] = pix[2] = pix[3] = value;
            }
        }
    }
    gEffectSuite->clipReleaseImage(image);

    return stat;
}

////////////////////////////////////////////////////////////////////////////////
// net.natron.test.Generator

OfxStatus
generatorPluginMain(const char* action,
                    const void* handle,
                    OfxPropertySetHandle inArgs,
                    OfxPropertySetHandle outArgs)
{
    OfxImageEffectHandle effect = (OfxImageEffectHandle)handle;

    if (std::strcmp(action, kOfxActionDescribe) == 0) {
        OfxStatus stat = describe(effect, "GeneratorTest", kOfxImageEffectRenderFullySafe, false, kOfxImageEffectContextGenerator);
        if (stat != kOfxStatOK) {
            return stat;
        }
        OfxPropertySetHandle props;
        gEffectSuite->getPropertySet(effect, &props);
        // so that it may set its output PAR in getClipPreferences
        gPropSuite->propSetInt(props, kOfxImageEffectPropSupportsMultipleClipPARs, 0, 1);
        // and getClipPreferences is called again when "par" or "fps" changes
        gPropSuite->propSetString(props, kOfxImageEffectPropClipPreferencesSlaveParam, 0, "par");
        gPropSuite->propSetString(props, kOfxImageEffectPropClipPreferencesSlaveParam, 1, "fps");

        return kOfxStatOK;
    }
    if (std::strcmp(action, kOfxImageEffectActionDescribeInContext) == 0) {
        OfxPropertySetHandle props;
        OfxStatus stat = gEffectSuite->clipDefine(effect, kOfxImageEffectOutputClipName, &props);
        if (stat != kOfxStatOK) {
            return stat;
        }
        gPropSuite->propSetString(props, kOfxImageEffectPropSupportedComponents, 0, kOfxImageComponentRGBA);
        OfxParamSetHandle paramSet = getParamSet(effect);
        if ( !paramSet || (gParamSuite->paramDefine(paramSet, kOfxParamTypeDouble, "par", &props) != kOfxStatOK) ) {
            return kOfxStatFailed;
        }
        gPropSuite->propSetDouble(props, kOfxParamPropDefault, 0, 1.);
        if (gParamSuite->paramDefine(paramSet, kOfxParamTypeDouble, "fps", &props) != kOfxStatOK) {
            return kOfxStatFailed;
        }
        gPropSuite->propSetDouble(props, kOfxParamPropDefault, 0, 0.);

        return kOfxStatOK;
    }
    if (std::strcmp(action, kOfxImageEffectActionGetClipPreferences) == 0) {
        double par = 1.;
        OfxParamHandle param = getParam(effect, "par");
        if (param) {
            gParamSuite->paramGetValue(param, &par);
        }
        gPropSuite->propSetDouble(outArgs, "OfxImageClipPropPAR_" kOfxImageEffectOutputClipName, 0, par);
        double fps = 0.;
        param = getParam(effect, "fps");
        if (param) {
            gParamSuite->paramGetValue(param, &fps);
        }
        if (fps > 0.) {
            gPropSuite->propSetDouble(outArgs, kOfxImageEffectPropFrameRate, 0, fps);
        }

        return kOfxStatOK;
    }
    if (std::strcmp(action, kOfxImageEffectActionRender) == 0) {
        incrementCounter("Generator.render");

        return fillOutput(effect, inArgs, 0.5f);
    }

    return commonAction(action, effect, "Generator");
}

////////////////////////////////////////////////////////////////////////////////
// net.natron.test.MultiPAR

// Record the PAR of the clip and of its image at time.
void
recordPAR(OfxImageEffectHandle effect,
          const char* clipName,
          OfxTime time)
{
    OfxImageClipHandle clip = getClip(effect, clipName);
    OfxPropertySetHandle props;

    if ( !clip || (gEffectSuite->clipGetPropertySet(clip, &props) != kOfxStatOK) ) {
        return;
    }
    double par = 0.;
    if (gPropSuite->propGetDouble(props, kOfxImagePropPixelAspectRatio, 0, &par) == kOfxStatOK) {
        setCounter(std::string("MultiPAR.") + clipName + "ClipPAR", (int)(par * 1000. + 0.5) );
    }
    OfxPropertySetHandle image = 0;
    if (gEffectSuite->clipGetImage(clip, time, 0, &image) == kOfxStatOK) {
        par = 0.;
        if (gPropSuite->propGetDouble(image, kOfxImagePropPixelAspectRatio, 0, &par) == kOfxStatOK) {
            setCounter(std::string("MultiPAR.") + clipName + "ImagePAR", (int)(par * 1000. + 0.5) );
        }
        gEffectSuite->clipReleaseImage(image);
    }
}

OfxStatus
multiPARPluginMain(const char* action,
                   const void* handle,
                   OfxPropertySetHandle inArgs,
                   OfxPropertySetHandle /*outArgs*/)
{
    OfxImageEffectHandle effect = (OfxImageEffectHandle)handle;

    if (std::strcmp(action, kOfxActionDescribe) == 0) {
        OfxStatus stat = describe(effect, "MultiPARTest", kOfxImageEffectRenderFullySafe, false, kOfxImageEffectContextGeneral);
        if (stat != kOfxStatOK) {
            return stat;
        }
        OfxPropertySetHandle props;
        gEffectSuite->getPropertySet(effect, &props);
        gPropSuite->propSetInt(props, kOfxImageEffectPropSupportsMultipleClipPARs, 0, 1);

        return kOfxStatOK;
    }
    if (std::strcmp(action, kOfxImageEffectActionDescribeInContext) == 0) {
        OfxStatus stat = defineClips(effect);
        if (stat != kOfxStatOK) {
            return stat;
        }
        OfxPropertySetHandle props;
        stat = gEffectSuite->clipDefine(effect, "B", &props);
        if (stat != kOfxStatOK) {
            return stat;
        }
        gPropSuite->propSetString(props, kOfxImageEffectPropSupportedComponents, 0, kOfxImageComponentRGBA);
        gPropSuite->propSetInt(props, kOfxImageClipPropOptional, 0, 1);

        return kOfxStatOK;
    }
    if (std::strcmp(action, kOfxImageEffectActionRender) == 0) {
        incrementCounter("MultiPAR.render");
        OfxTime time = 0.;
        gPropSuite->propGetDouble(inArgs, kOfxPropTime, 0, &time);
        recordPAR(effect, kOfxImageEffectSimpleSourceClipName, time);
        recordPAR(effect, "B", time);

        return fillOutput(effect, inArgs, 0.f);
    }

    return commonAction(action, effect, "MultiPAR");
}

////////////////////////////////////////////////////////////////////////////////
// net.natron.test.ClipGetImage

// the regions fetched in render, in canonical coordinates (the tests render at scale 1 with PAR 1, so
// these are also pixel coordinates)
const OfxRectI kClipGetImageSmall = { 0, 0, 8, 8 };
const OfxRectI kClipGetImageLarge = { 0, 0, 48, 48 };

OfxRectI
intersection(const OfxRectI& a,
             const OfxRectD& b)
{
    OfxRectI r;

    r.x1 = a.x1 > (int)b.x1 ? a.x1 : (int)b.x1;
    r.y1 = a.y1 > (int)b.y1 ? a.y1 : (int)b.y1;
    r.x2 = a.x2 < (int)b.x2 ? a.x2 : (int)b.x2;
    r.y2 = a.y2 < (int)b.y2 ? a.y2 : (int)b.y2;

    return r;
}

// Fetch region of Source at time, and check that the image covers it (clipped to the RoD).
OfxPropertySetHandle
fetchAndCheck(OfxImageClipHandle source,
              OfxTime time,
              const OfxRectI& region)
{
    OfxRectD regionD = { (double)region.x1, (double)region.y1, (double)region.x2, (double)region.y2 };
    OfxPropertySetHandle image = 0;

    if (gEffectSuite->clipGetImage(source, time, &regionD, &image) != kOfxStatOK) {
        incrementCounter("ClipGetImage.fetchFailed");

        return 0;
    }
    OfxRectD rod;
    OfxRectI bounds;
    if ( (gEffectSuite->clipGetRegionOfDefinition(source, time, &rod) != kOfxStatOK) ||
         (gPropSuite->propGetIntN(image, kOfxImagePropBounds, 4, &bounds.x1) != kOfxStatOK) ||
         !contains( bounds, intersection(region, rod) ) ) {
        incrementCounter("ClipGetImage.regionErrors");
    }

    return image;
}

OfxStatus
clipGetImagePluginMain(const char* action,
                       const void* handle,
                       OfxPropertySetHandle inArgs,
                       OfxPropertySetHandle outArgs)
{
    OfxImageEffectHandle effect = (OfxImageEffectHandle)handle;

    if (std::strcmp(action, kOfxActionDescribe) == 0) {
        return describe(effect, "ClipGetImageTest", kOfxImageEffectRenderFullySafe, false);
    }
    if (std::strcmp(action, kOfxImageEffectActionDescribeInContext) == 0) {
        return defineClips(effect);
    }
    if (std::strcmp(action, kOfxImageEffectActionGetRegionsOfInterest) == 0) {
        // only the small region is needed, so that the host prepares only that part of the input
        double roi[4] = { (double)kClipGetImageSmall.x1, (double)kClipGetImageSmall.y1,
                          (double)kClipGetImageSmall.x2, (double)kClipGetImageSmall.y2 };
        gPropSuite->propSetDoubleN(outArgs, "OfxImageClipPropRoI_" kOfxImageEffectSimpleSourceClipName, 4, roi);

        return kOfxStatOK;
    }
    if (std::strcmp(action, kOfxImageEffectActionRender) == 0) {
        incrementCounter("ClipGetImage.render");
        OfxTime time = 0.;
        gPropSuite->propGetDouble(inArgs, kOfxPropTime, 0, &time);
        OfxImageClipHandle source = getClip(effect, kOfxImageEffectSimpleSourceClipName);
        if (!source) {
            return kOfxStatFailed;
        }
        OfxPropertySetHandle small = fetchAndCheck(source, time, kClipGetImageSmall);
        OfxPropertySetHandle large = fetchAndCheck(source, time, kClipGetImageLarge);
        // "If clipGetImage is called twice with the same parameters, then two separate image handles will
        // be returned, each of which must be release."
        OfxPropertySetHandle large2 = fetchAndCheck(source, time, kClipGetImageLarge);
        if ( large && (large == large2) ) {
            incrementCounter("ClipGetImage.sameHandle");
        }
        incrementCounter("ClipGetImage.checks");
        if (small) {
            gEffectSuite->clipReleaseImage(small);
        }
        if (large) {
            gEffectSuite->clipReleaseImage(large);
        }
        if (large2) {
            gEffectSuite->clipReleaseImage(large2);
        }

        return fillOutput(effect, inArgs, 0.f);
    }

    return commonAction(action, effect, "ClipGetImage");
}

////////////////////////////////////////////////////////////////////////////////
// net.natron.test.Sequential and net.natron.test.SequentialStatus

struct SequentialData
{
    int lastSequentialFrame; // the frame of the last render with kOfxImageEffectPropSequentialRenderStatus 1
};

SequentialData*
getSequentialData(OfxImageEffectHandle effect)
{
    OfxPropertySetHandle props;
    void* data = 0;

    if ( (gEffectSuite->getPropertySet(effect, &props) != kOfxStatOK) ||
         (gPropSuite->propGetPointer(props, kOfxPropInstanceData, 0, &data) != kOfxStatOK) ) {
        return 0;
    }

    return (SequentialData*)data;
}

OfxStatus
sequentialMain(const char* action,
               OfxImageEffectHandle effect,
               OfxPropertySetHandle inArgs,
               OfxPropertySetHandle outArgs,
               const std::string& id,
               int sequentialRender)
{
    if (std::strcmp(action, kOfxActionDescribe) == 0) {
        OfxStatus stat = describe(effect, (id + "Test").c_str(), kOfxImageEffectRenderFullySafe, false);
        if (stat != kOfxStatOK) {
            return stat;
        }
        OfxPropertySetHandle props;
        gEffectSuite->getPropertySet(effect, &props);
        gPropSuite->propSetInt(props, kOfxImageEffectInstancePropSequentialRender, 0, sequentialRender);

        return kOfxStatOK;
    }
    if (std::strcmp(action, kOfxImageEffectActionDescribeInContext) == 0) {
        OfxStatus stat = defineClips(effect);
        if (stat != kOfxStatOK) {
            return stat;
        }
        OfxParamSetHandle paramSet = getParamSet(effect);
        OfxPropertySetHandle props;
        if ( !paramSet || (gParamSuite->paramDefine(paramSet, kOfxParamTypeInteger, "instanceSequential", &props) != kOfxStatOK) ) {
            return kOfxStatFailed;
        }
        gPropSuite->propSetInt(props, kOfxParamPropDefault, 0, -1); // -1: keep the descriptor's value

        return kOfxStatOK;
    }
    if (std::strcmp(action, kOfxActionCreateInstance) == 0) {
        OfxPropertySetHandle props;
        if (gEffectSuite->getPropertySet(effect, &props) != kOfxStatOK) {
            return kOfxStatFailed;
        }
        SequentialData* data = new SequentialData;
        data->lastSequentialFrame = INT_MIN;
        gPropSuite->propSetPointer(props, kOfxPropInstanceData, 0, data);
    } else if (std::strcmp(action, kOfxActionDestroyInstance) == 0) {
        delete getSequentialData(effect);
    } else if (std::strcmp(action, kOfxActionInstanceChanged) == 0) {
        if (changedParamName(inArgs) == "instanceSequential") {
            int value = -1;
            OfxParamHandle param = getParam(effect, "instanceSequential");
            OfxPropertySetHandle props;
            if ( param && (gParamSuite->paramGetValue(param, &value) == kOfxStatOK) && (value >= 0) &&
                 (gEffectSuite->getPropertySet(effect, &props) == kOfxStatOK) ) {
                // kOfxImageEffectInstancePropSequentialRender is read/write on the instance
                gPropSuite->propSetInt(props, kOfxImageEffectInstancePropSequentialRender, 0, value);
            }
        }

        return kOfxStatOK;
    } else if (std::strcmp(action, kOfxImageEffectActionGetClipPreferences) == 0) {
        // every frame is different, so that the host renders each of them
        gPropSuite->propSetInt(outArgs, kOfxImageEffectFrameVarying, 0, 1);

        return kOfxStatOK;
    } else if (std::strcmp(action, kOfxImageEffectActionRender) == 0) {
        OfxTime time = 0.;
        int status = 0;
        gPropSuite->propGetDouble(inArgs, kOfxPropTime, 0, &time);
        gPropSuite->propGetInt(inArgs, kOfxImageEffectPropSequentialRenderStatus, 0, &status);
        SequentialData* data = getSequentialData(effect);
        if (!data) {
            return kOfxStatFailed;
        }
        const int frame = (int)time;
        {
            std::lock_guard<std::mutex> l(gMutex);
            int& active = gCounters[id + ".activeRenders"];
            ++active;
            int& maxActive = gCounters[id + ".maxActiveRenders"];
            if (active > maxActive) {
                maxActive = active;
            }
        }
        // Every third frame takes longer to start: frames rendered in parallel reach the check below out of
        // frame order, which a render in frame order on a single instance never does.
        std::this_thread::sleep_for( std::chrono::milliseconds(frame % 3 == 0 ? 40 : 5) );
        {
            std::lock_guard<std::mutex> l(gMutex);
            if (status) {
                ++gCounters[id + ".sequentialRenders"];
                // the same frame again is not out of order; any other frame than the next one is
                if ( (data->lastSequentialFrame != INT_MIN) && (frame != data->lastSequentialFrame) &&
                     (frame != data->lastSequentialFrame + 1) ) {
                    ++gCounters[id + ".outOfOrder"];
                }
                data->lastSequentialFrame = frame;
            } else {
                ++gCounters[id + ".nonSequentialRenders"];
            }
        }
        OfxStatus stat = fillOutput(effect, inArgs, (float)time);
        {
            std::lock_guard<std::mutex> l(gMutex);
            --gCounters[id + ".activeRenders"];
        }

        return stat;
    }

    return commonAction(action, effect, id);
}

OfxStatus
sequentialPluginMain(const char* action,
                     const void* handle,
                     OfxPropertySetHandle inArgs,
                     OfxPropertySetHandle outArgs)
{
    return sequentialMain(action, (OfxImageEffectHandle)handle, inArgs, outArgs, "Sequential", 1);
}

OfxStatus
sequentialStatusPluginMain(const char* action,
                           const void* handle,
                           OfxPropertySetHandle inArgs,
                           OfxPropertySetHandle outArgs)
{
    return sequentialMain(action, (OfxImageEffectHandle)handle, inArgs, outArgs, "SequentialStatus", 0);
}

////////////////////////////////////////////////////////////////////////////////
// net.natron.test.TimeBlur

OfxStatus
timeBlurPluginMain(const char* action,
                   const void* handle,
                   OfxPropertySetHandle inArgs,
                   OfxPropertySetHandle outArgs)
{
    OfxImageEffectHandle effect = (OfxImageEffectHandle)handle;

    if (std::strcmp(action, kOfxActionDescribe) == 0) {
        OfxStatus stat = describe(effect, "TimeBlurTest", kOfxImageEffectRenderFullySafe, false);
        if (stat != kOfxStatOK) {
            return stat;
        }
        OfxPropertySetHandle props;
        gEffectSuite->getPropertySet(effect, &props);
        gPropSuite->propSetInt(props, kOfxImageEffectPropTemporalClipAccess, 0, 1);

        return kOfxStatOK;
    }
    if (std::strcmp(action, kOfxImageEffectActionDescribeInContext) == 0) {
        return defineClips(effect);
    }
    if (std::strcmp(action, kOfxImageEffectActionGetFramesNeeded) == 0) {
        OfxTime time = 0.;
        gPropSuite->propGetDouble(inArgs, kOfxPropTime, 0, &time);
        double range[2] = { time - 1., time + 1. };
        gPropSuite->propSetDoubleN(outArgs, "OfxImageClipPropFrameRange_" kOfxImageEffectSimpleSourceClipName, 2, range);

        return kOfxStatOK;
    }
    if (std::strcmp(action, kOfxImageEffectActionRender) == 0) {
        incrementCounter("TimeBlur.render");
        OfxTime time = 0.;
        gPropSuite->propGetDouble(inArgs, kOfxPropTime, 0, &time);
        OfxImageClipHandle source = getClip(effect, kOfxImageEffectSimpleSourceClipName);
        if (!source) {
            return kOfxStatFailed;
        }
        for (int dt = -1; dt <= 1; ++dt) {
            OfxPropertySetHandle image = 0;
            if (gEffectSuite->clipGetImage(source, time + dt, 0, &image) == kOfxStatOK) {
                gEffectSuite->clipReleaseImage(image);
            } else {
                incrementCounter("TimeBlur.fetchFailed");
            }
        }

        return fillOutput(effect, inArgs, 0.f);
    }

    return commonAction(action, effect, "TimeBlur");
}

////////////////////////////////////////////////////////////////////////////////
// net.natron.test.GPURenderFailed

OfxStatus
gpuRenderFailedMain(const char* action,
                    OfxImageEffectHandle effect,
                    OfxPropertySetHandle inArgs,
                    const std::string& id,
                    bool cpuRenderSupported)
{
    if (std::strcmp(action, kOfxActionDescribe) == 0) {
        OfxStatus stat = describe(effect, (id + "Test").c_str(), kOfxImageEffectRenderFullySafe, false);
        if (stat != kOfxStatOK) {
            return stat;
        }
        OfxPropertySetHandle props;
        gEffectSuite->getPropertySet(effect, &props);
        gPropSuite->propSetString(props, kOfxImageEffectPropOpenGLRenderSupported, 0, "true");
        gPropSuite->propSetString(props, kOfxImageEffectPropCPURenderSupported, 0, cpuRenderSupported ? "true" : "false");

        return kOfxStatOK;
    }
    if (std::strcmp(action, kOfxImageEffectActionDescribeInContext) == 0) {
        return defineClips(effect);
    }
    if (std::strcmp(action, kOfxImageEffectActionRender) == 0) {
        int openGLEnabled = 0;
        if ( (gPropSuite->propGetInt(inArgs, kOfxImageEffectPropOpenGLEnabled, 0, &openGLEnabled) == kOfxStatOK) &&
             openGLEnabled ) {
            incrementCounter(id + ".glRenders");

            // "the host should retry the render with OpenGL rendering disabled" (ofxGPURender.h)
            return kOfxStatGPURenderFailed;
        }
        incrementCounter(id + ".cpuRenders");

        return fillOutput(effect, inArgs, 1.f);
    }

    return commonAction(action, effect, id);
}

OfxStatus
gpuRenderFailedPluginMain(const char* action,
                          const void* handle,
                          OfxPropertySetHandle inArgs,
                          OfxPropertySetHandle /*outArgs*/)
{
    return gpuRenderFailedMain(action, (OfxImageEffectHandle)handle, inArgs, "GPURenderFailed", true);
}

OfxStatus
gpuOnlyRenderFailedPluginMain(const char* action,
                              const void* handle,
                              OfxPropertySetHandle inArgs,
                              OfxPropertySetHandle /*outArgs*/)
{
    return gpuRenderFailedMain(action, (OfxImageEffectHandle)handle, inArgs, "GPUOnlyRenderFailed", false);
}

////////////////////////////////////////////////////////////////////////////////
// net.natron.test.Question

void
recordReply(const std::string& where,
            OfxStatus stat)
{
    incrementCounter( "Question." + where + (stat == kOfxStatReplyYes ? ".yes" : stat == kOfxStatReplyNo ? ".no" : ".error") );
}

OfxStatus
questionPluginMain(const char* action,
                   const void* handle,
                   OfxPropertySetHandle inArgs,
                   OfxPropertySetHandle /*outArgs*/)
{
    OfxImageEffectHandle effect = (OfxImageEffectHandle)handle;

    if (std::strcmp(action, kOfxActionDescribe) == 0) {
        return describe(effect, "QuestionTest", kOfxImageEffectRenderFullySafe, false);
    }
    if (std::strcmp(action, kOfxImageEffectActionDescribeInContext) == 0) {
        OfxStatus stat = defineClips(effect);
        if (stat != kOfxStatOK) {
            return stat;
        }
        OfxParamSetHandle paramSet = getParamSet(effect);
        OfxPropertySetHandle props;
        if ( !paramSet ||
             (gParamSuite->paramDefine(paramSet, kOfxParamTypePushButton, "ask", &props) != kOfxStatOK) ||
             (gParamSuite->paramDefine(paramSet, kOfxParamTypePushButton, "askFromThread", &props) != kOfxStatOK) ) {
            return kOfxStatFailed;
        }

        return kOfxStatOK;
    }
    if (std::strcmp(action, kOfxActionInstanceChanged) == 0) {
        const std::string name = changedParamName(inArgs);
        if (name == "ask") {
            recordReply( "ask", gMessageSuite->message(effect, kOfxMessageQuestion, "", "Do you really want to?") );
        } else if (name == "askFromThread") {
            OfxStatus stat = kOfxStatFailed;
            std::thread thread([effect, &stat]() {
                stat = gMessageSuite->message(effect, kOfxMessageQuestion, "", "Do you really want to (from a thread)?");
            });
            thread.join();
            recordReply("askFromThread", stat);
        }

        return kOfxStatOK;
    }

    return commonAction(action, effect, "Question");
}

void
setHost(OfxHost* host)
{
    gHost = host;
}

OfxPlugin gPlugins[] = {
    { kOfxImageEffectPluginApi, 1, kParamInteractPluginIdentifier, 1, 0, setHost, paramInteractPluginMain },
    { kOfxImageEffectPluginApi, 1, kSingleInstancePluginIdentifier, 1, 0, setHost, singleInstancePluginMain },
    { kOfxImageEffectPluginApi, 1, kSyncPrivateDataPluginIdentifier, 1, 0, setHost, syncPrivateDataPluginMain },
    { kOfxImageEffectPluginApi, 1, kParamsPluginIdentifier, 1, 0, setHost, paramsPluginMain },
    { kOfxImageEffectPluginApi, 1, kGeneratorPluginIdentifier, 1, 0, setHost, generatorPluginMain },
    { kOfxImageEffectPluginApi, 1, kMultiPARPluginIdentifier, 1, 0, setHost, multiPARPluginMain },
    { kOfxImageEffectPluginApi, 1, kClipGetImagePluginIdentifier, 1, 0, setHost, clipGetImagePluginMain },
    { kOfxImageEffectPluginApi, 1, kSequentialPluginIdentifier, 1, 0, setHost, sequentialPluginMain },
    { kOfxImageEffectPluginApi, 1, kSequentialStatusPluginIdentifier, 1, 0, setHost, sequentialStatusPluginMain },
    { kOfxImageEffectPluginApi, 1, kTimeBlurPluginIdentifier, 1, 0, setHost, timeBlurPluginMain },
    { kOfxImageEffectPluginApi, 1, kGPURenderFailedPluginIdentifier, 1, 0, setHost, gpuRenderFailedPluginMain },
    { kOfxImageEffectPluginApi, 1, kGPUOnlyRenderFailedPluginIdentifier, 1, 0, setHost, gpuOnlyRenderFailedPluginMain },
    { kOfxImageEffectPluginApi, 1, kQuestionPluginIdentifier, 1, 0, setHost, questionPluginMain },
};
} // anonymous namespace

std::vector<OfxPlugin*>&
natronTestExtraPlugins()
{
    static std::vector<OfxPlugin*> plugins;

    return plugins;
}

void
natronTestIncrementCounter(const std::string& name,
                           int delta)
{
    incrementCounter(name, delta);
}

extern "C" {
// For the tests (Tests/OfxTestPlugin.h): the value of a counter, 0 if it was never set.
OfxExport int NatronTestPluginGetCounter(const char* name);

// OFX 1.5: "The order of calls is then: 1) OfxSetHost, 2) OfxGetNumberOfPlugins, 3) OfxGetPlugin"
OfxExport OfxStatus
OfxSetHost(const OfxHost* host)
{
    int apiVersion[2] = { 0, 0 };

    if (host) {
        const OfxPropertySuiteV1* propSuite = (const OfxPropertySuiteV1*)host->fetchSuite(host->host, kOfxPropertySuite, 1);
        if (propSuite) {
            propSuite->propGetIntN(host->host, kOfxPropAPIVersion, 2, apiVersion);
        }
    }
    std::lock_guard<std::mutex> l(gMutex);
    ++gCounters["Binary.setHost"];
    if (gCounters["Binary.getNumberOfPlugins"] == 0) {
        ++gCounters["Binary.setHostBeforeGetNumberOfPlugins"];
    }
    gCounters["Binary.setHostAPIVersion0"] = apiVersion[0];
    gCounters["Binary.setHostAPIVersion1"] = apiVersion[1];

    return kOfxStatOK;
}

OfxExport int
OfxGetNumberOfPlugins(void)
{
    incrementCounter("Binary.getNumberOfPlugins");

    // the plug-ins of this file, then the ones of the other source files (NatronTestPluginRegistry.h)
    return (int)( sizeof(gPlugins) / sizeof(gPlugins[0]) + natronTestExtraPlugins().size() );
}

OfxExport OfxPlugin*
OfxGetPlugin(int nth)
{
    const int nOwn = (int)( sizeof(gPlugins) / sizeof(gPlugins[0]) );

    // (not OfxGetNumberOfPlugins(), which counts its calls)
    if ( (nth < 0) || ( nth >= nOwn + (int)natronTestExtraPlugins().size() ) ) {
        return 0;
    }

    return nth < nOwn ? &gPlugins[nth] : natronTestExtraPlugins()[nth - nOwn];
}

OfxExport int
NatronTestPluginGetCounter(const char* name)
{
    std::lock_guard<std::mutex> l(gMutex);
    std::map<std::string, int>::const_iterator found = gCounters.find(name);

    return found == gCounters.end() ? 0 : found->second;
}
// For the tests (Tests/OfxTestPlugin.h): set a counter, e.g. to reset a maximum.
OfxExport void NatronTestPluginSetCounter(const char* name, int value);

OfxExport void
NatronTestPluginSetCounter(const char* name,
                           int value)
{
    std::lock_guard<std::mutex> l(gMutex);
    gCounters[name] = value;
}
} // extern "C"
