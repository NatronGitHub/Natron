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

// Plug-ins with contexts Natron does not support (Tests/OfxContexts_Test.cpp). Each one counts its
// kOfxImageEffectActionDescribeInContext actions by context ("<name>.describe.<context>"):
// - net.natron.test.RetimerOnly: only the Retimer context, which Natron does not support: it must not
//   be loaded ("If a plug-in does not report to support any context supported by host, host should not
//   enable the plug-in", ofxCore.h);
// - net.natron.test.RetimerOrFilter: Retimer and Filter: it must be described and created as a Filter;
// - net.natron.test.ContextFallback: General and Filter, but its General description returns
//   kOfxStatErrMissingHostFeature ("the context will be ignored by the host", ofxImageEffect.h): it
//   must be created as a Filter.

#include <cstring>
#include <string>

#include <ofxCore.h>
#include <ofxImageEffect.h>
#include <ofxProperty.h>

#include "NatronTestPluginRegistry.h"

namespace {
OfxHost* gHost = 0;
const OfxPropertySuiteV1* gPropSuite = 0;
const OfxImageEffectSuiteV1* gEffectSuite = 0;

struct ContextsPlugin
{
    const char* name; // counter prefix and label
    const char* contexts[2]; // supported contexts (NULL-terminated if fewer)
    const char* missingFeatureContext; // DescribeInContext returns kOfxStatErrMissingHostFeature in this one
};

const ContextsPlugin gRetimerOnly = { "RetimerOnly", { kOfxImageEffectContextRetimer, 0 }, 0 };
const ContextsPlugin gRetimerOrFilter = { "RetimerOrFilter", { kOfxImageEffectContextRetimer, kOfxImageEffectContextFilter }, 0 };
const ContextsPlugin gContextFallback = { "ContextFallback", { kOfxImageEffectContextGeneral, kOfxImageEffectContextFilter },
                                          kOfxImageEffectContextGeneral };

OfxStatus
describe(const ContextsPlugin& plugin,
         OfxImageEffectHandle effect)
{
    OfxPropertySetHandle props;
    OfxStatus stat = gEffectSuite->getPropertySet(effect, &props);

    if (stat != kOfxStatOK) {
        return stat;
    }
    gPropSuite->propSetString(props, kOfxPropLabel, 0, plugin.name);
    gPropSuite->propSetString(props, kOfxImageEffectPluginPropGrouping, 0, "Other/Test");
    for (int i = 0; i < 2 && plugin.contexts[i]; ++i) {
        gPropSuite->propSetString(props, kOfxImageEffectPropSupportedContexts, i, plugin.contexts[i]);
    }
    gPropSuite->propSetString(props, kOfxImageEffectPropSupportedPixelDepths, 0, kOfxBitDepthFloat);
    gPropSuite->propSetString(props, kOfxImageEffectPluginRenderThreadSafety, 0, kOfxImageEffectRenderFullySafe);

    return kOfxStatOK;
}

OfxStatus
describeInContext(const ContextsPlugin& plugin,
                  OfxImageEffectHandle effect,
                  OfxPropertySetHandle inArgs)
{
    const char* context = 0;

    if ( !inArgs || (gPropSuite->propGetString(inArgs, kOfxImageEffectPropContext, 0, &context) != kOfxStatOK) || !context ) {
        return kOfxStatFailed;
    }
    natronTestIncrementCounter(std::string(plugin.name) + ".describe." + context);
    if ( plugin.missingFeatureContext && (std::strcmp(context, plugin.missingFeatureContext) == 0) ) {
        return kOfxStatErrMissingHostFeature;
    }
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

OfxStatus
contextsPluginMain(const ContextsPlugin& plugin,
                   const char* action,
                   const void* handle,
                   OfxPropertySetHandle inArgs)
{
    OfxImageEffectHandle effect = (OfxImageEffectHandle)handle;

    if (std::strcmp(action, kOfxActionLoad) == 0) {
        if (!gHost) {
            return kOfxStatErrMissingHostFeature;
        }
        gPropSuite = (const OfxPropertySuiteV1*)gHost->fetchSuite(gHost->host, kOfxPropertySuite, 1);
        gEffectSuite = (const OfxImageEffectSuiteV1*)gHost->fetchSuite(gHost->host, kOfxImageEffectSuite, 1);

        return (gPropSuite && gEffectSuite) ? kOfxStatOK : kOfxStatErrMissingHostFeature;
    }
    if (std::strcmp(action, kOfxActionDescribe) == 0) {
        return describe(plugin, effect);
    }
    if (std::strcmp(action, kOfxImageEffectActionDescribeInContext) == 0) {
        return describeInContext(plugin, effect, inArgs);
    }

    return kOfxStatReplyDefault;
}

OfxStatus
retimerOnlyMain(const char* action,
                const void* handle,
                OfxPropertySetHandle inArgs,
                OfxPropertySetHandle /*outArgs*/)
{
    return contextsPluginMain(gRetimerOnly, action, handle, inArgs);
}

OfxStatus
retimerOrFilterMain(const char* action,
                    const void* handle,
                    OfxPropertySetHandle inArgs,
                    OfxPropertySetHandle /*outArgs*/)
{
    return contextsPluginMain(gRetimerOrFilter, action, handle, inArgs);
}

OfxStatus
contextFallbackMain(const char* action,
                    const void* handle,
                    OfxPropertySetHandle inArgs,
                    OfxPropertySetHandle /*outArgs*/)
{
    return contextsPluginMain(gContextFallback, action, handle, inArgs);
}

void
setHost(OfxHost* host)
{
    gHost = host;
}

OfxPlugin gPlugins[] = {
    { kOfxImageEffectPluginApi, 1, "net.natron.test.RetimerOnly", 1, 0, setHost, retimerOnlyMain },
    { kOfxImageEffectPluginApi, 1, "net.natron.test.RetimerOrFilter", 1, 0, setHost, retimerOrFilterMain },
    { kOfxImageEffectPluginApi, 1, "net.natron.test.ContextFallback", 1, 0, setHost, contextFallbackMain },
};
NatronTestPluginRegistrar gRegistrars[] = {
    NatronTestPluginRegistrar(&gPlugins[0]), NatronTestPluginRegistrar(&gPlugins[1]), NatronTestPluginRegistrar(&gPlugins[2]),
};
} // anonymous namespace
