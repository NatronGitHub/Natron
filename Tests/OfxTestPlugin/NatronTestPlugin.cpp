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

#include <cstring>
#include <map>
#include <mutex>
#include <set>
#include <string>

#include <ofxCore.h>
#include <ofxImageEffect.h>
#include <ofxInteract.h>
#include <ofxParam.h>
#include <ofxProperty.h>

#define kPluginGrouping "Other/Test"
#define kParamInteractPluginIdentifier "net.natron.test.ParamInteract"
#define kSingleInstancePluginIdentifier "net.natron.test.SingleInstance"
#define kSyncPrivateDataPluginIdentifier "net.natron.test.SyncPrivateData"

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

void
setHost(OfxHost* host)
{
    gHost = host;
}

OfxPlugin gPlugins[] = {
    { kOfxImageEffectPluginApi, 1, kParamInteractPluginIdentifier, 1, 0, setHost, paramInteractPluginMain },
    { kOfxImageEffectPluginApi, 1, kSingleInstancePluginIdentifier, 1, 0, setHost, singleInstancePluginMain },
    { kOfxImageEffectPluginApi, 1, kSyncPrivateDataPluginIdentifier, 1, 0, setHost, syncPrivateDataPluginMain },
};
} // anonymous namespace

extern "C" {
// For the tests (Tests/OfxTestPlugin.h): the value of a counter, 0 if it was never set.
OfxExport int NatronTestPluginGetCounter(const char* name);

OfxExport int
OfxGetNumberOfPlugins(void)
{
    return (int)( sizeof(gPlugins) / sizeof(gPlugins[0]) );
}

OfxExport OfxPlugin*
OfxGetPlugin(int nth)
{
    if ( (nth < 0) || ( nth >= OfxGetNumberOfPlugins() ) ) {
        return 0;
    }

    return &gPlugins[nth];
}

OfxExport int
NatronTestPluginGetCounter(const char* name)
{
    std::lock_guard<std::mutex> l(gMutex);
    std::map<std::string, int>::const_iterator found = gCounters.find(name);

    return found == gCounters.end() ? 0 : found->second;
}
} // extern "C"
