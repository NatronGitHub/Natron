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
//   instances.
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
//   - "uiOnly" has kOfxParamPropEvaluateOnChange=0, "noUndo" has kOfxParamPropCanUndo=0.
//
// The binary exports OfxSetHost (OFX 1.5), and counts its calls ("Binary.setHost"), whether it came
// before OfxGetNumberOfPlugins ("Binary.setHostBeforeGetNumberOfPlugins") and the kOfxPropAPIVersion
// of the host it gets ("Binary.setHostAPIVersion0", "Binary.setHostAPIVersion1"). ParamInteract
// calls clipGetHandle() in kOfxImageEffectActionDescribeInContext, where it is not valid, and records
// the status and whether the clip was set to NULL ("ParamInteract.describeClipGetHandle*").

#include <cstdio>
#include <cstring>
#include <map>
#include <mutex>
#include <set>
#include <string>
#include <vector>

#include <ofxCore.h>
#include <ofxImageEffect.h>
#include <ofxInteract.h>
#include <ofxParam.h>
#include <ofxParametricParam.h>
#include <ofxProperty.h>

#include "NatronTestPluginRegistry.h"

#define kPluginGrouping "Other/Test"
#define kParamInteractPluginIdentifier "net.natron.test.ParamInteract"
#define kSingleInstancePluginIdentifier "net.natron.test.SingleInstance"
#define kSyncPrivateDataPluginIdentifier "net.natron.test.SyncPrivateData"
#define kParamsPluginIdentifier "net.natron.test.Params"

namespace {
OfxHost* gHost = 0;
const OfxPropertySuiteV1* gPropSuite = 0;
const OfxImageEffectSuiteV1* gEffectSuite = 0;
const OfxParameterSuiteV1* gParamSuite = 0;
const OfxInteractSuiteV1* gInteractSuite = 0;

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
    if (!gPropSuite || !gEffectSuite || !gParamSuite || !gInteractSuite) {
        return kOfxStatErrMissingHostFeature;
    }

    return kOfxStatOK;
}

OfxStatus
describe(OfxImageEffectHandle effect,
         const char* label,
         const char* threadSafety,
         bool singleInstance)
{
    OfxPropertySetHandle props;
    OfxStatus stat = gEffectSuite->getPropertySet(effect, &props);

    if (stat != kOfxStatOK) {
        return stat;
    }
    gPropSuite->propSetString(props, kOfxPropLabel, 0, label);
    gPropSuite->propSetString(props, kOfxImageEffectPluginPropGrouping, 0, kPluginGrouping);
    gPropSuite->propSetString(props, kOfxImageEffectPropSupportedContexts, 0, kOfxImageEffectContextFilter);
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

// Main entry of the actions common to all the plug-ins, kOfxStatReplyDefault for the others.
OfxStatus
commonAction(const char* action,
             OfxImageEffectHandle effect,
             const std::string& id)
{
    if (std::strcmp(action, kOfxActionLoad) == 0) {
        return onLoad();
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
    if (paramsDefine(paramSet, kOfxParamTypeBoolean, "uiOnly", &props) != kOfxStatOK) {
        return kOfxStatFailed;
    }
    gPropSuite->propSetInt(props, kOfxParamPropEvaluateOnChange, 0, 0);
    if (paramsDefine(paramSet, kOfxParamTypeDouble, "noUndo", &props) != kOfxStatOK) {
        return kOfxStatFailed;
    }
    gPropSuite->propSetInt(props, kOfxParamPropCanUndo, 0, 0);

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
} // extern "C"
