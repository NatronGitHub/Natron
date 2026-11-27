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

// net.natron.test.HostOverlayHandle (Tests/OfxHostOverlayHandle_Test.cpp, #1932): a filter with an overlay
// interact (which draws nothing) and four Double2D parameters, of type kOfxParamDoubleTypeXYAbsolute,
// kOfxParamDoubleTypeNormalisedXYAbsolute, kOfxParamDoubleTypeXY and kOfxParamDoubleTypePlain (the default,
// not set). In kOfxImageEffectActionDescribeInContext, after setting the type, it reads
// kOfxParamPropHasHostOverlayHandle on each descriptor and sets kOfxParamPropUseHostOverlayHandle to the
// same value, as a plug-in that asks for the host's handles only where the host has them (the OFX 1.2
// protocol, e.g. openfx-supportext's PositionInteract). Counters ("HostOverlayHandle.<param>.has"): the value
// read, or -1 if reading it failed.

#include <cstring>
#include <string>

#include <ofxCore.h>
#include <ofxImageEffect.h>
#include <ofxInteract.h>
#include <ofxOld.h> // kOfxParamDoubleTypeNormalisedXYAbsolute
#include <ofxParam.h>
#include <ofxProperty.h>

#include "NatronTestPluginRegistry.h"

namespace {
OfxHost* gHost = 0;
const OfxPropertySuiteV1* gPropSuite = 0;
const OfxImageEffectSuiteV1* gEffectSuite = 0;
const OfxParameterSuiteV1* gParamSuite = 0;

OfxStatus
overlayMain(const char* /*action*/,
            const void* /*handle*/,
            OfxPropertySetHandle /*inArgs*/,
            OfxPropertySetHandle /*outArgs*/)
{
    return kOfxStatReplyDefault;
}

// Define a Double2D parameter of the given type (none: the default), then read HasHostOverlayHandle and set
// UseHostOverlayHandle to it.
OfxStatus
defineDouble2D(OfxParamSetHandle paramSet,
               const char* name,
               const char* doubleType)
{
    OfxPropertySetHandle props = 0;
    OfxStatus stat = gParamSuite->paramDefine(paramSet, kOfxParamTypeDouble2D, name, &props);

    if (stat != kOfxStatOK) {
        return stat;
    }
    if (doubleType) {
        gPropSuite->propSetString(props, kOfxParamPropDoubleType, 0, doubleType);
    }
    int has = 0;
    if (gPropSuite->propGetInt(props, kOfxParamPropHasHostOverlayHandle, 0, &has) != kOfxStatOK) {
        has = -1;
    }
    natronTestSetCounter(std::string("HostOverlayHandle.") + name + ".has", has);
    gPropSuite->propSetInt(props, kOfxParamPropUseHostOverlayHandle, 0, has == 1 ? 1 : 0);

    return kOfxStatOK;
}

OfxStatus
pluginMain(const char* action,
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
        gParamSuite = (const OfxParameterSuiteV1*)gHost->fetchSuite(gHost->host, kOfxParameterSuite, 1);

        return (gPropSuite && gEffectSuite && gParamSuite) ? kOfxStatOK : kOfxStatErrMissingHostFeature;
    }
    if (std::strcmp(action, kOfxActionDescribe) == 0) {
        OfxPropertySetHandle props;
        OfxStatus stat = gEffectSuite->getPropertySet(effect, &props);
        if (stat != kOfxStatOK) {
            return stat;
        }
        gPropSuite->propSetString(props, kOfxPropLabel, 0, "HostOverlayHandle");
        gPropSuite->propSetString(props, kOfxImageEffectPluginPropGrouping, 0, "Other/Test");
        gPropSuite->propSetString(props, kOfxImageEffectPropSupportedContexts, 0, kOfxImageEffectContextFilter);
        gPropSuite->propSetString(props, kOfxImageEffectPropSupportedPixelDepths, 0, kOfxBitDepthFloat);
        gPropSuite->propSetString(props, kOfxImageEffectPluginRenderThreadSafety, 0, kOfxImageEffectRenderFullySafe);
        // with an overlay interact, Natron draws a native handle only where the plug-in asks for one
        gPropSuite->propSetPointer(props, kOfxImageEffectPluginPropOverlayInteractV1, 0, (void*)overlayMain);

        return kOfxStatOK;
    }
    if (std::strcmp(action, kOfxImageEffectActionDescribeInContext) == 0) {
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
        OfxParamSetHandle paramSet = 0;
        stat = gEffectSuite->getParamSet(effect, &paramSet);
        if (stat != kOfxStatOK) {
            return stat;
        }
        const struct
        {
            const char* name;
            const char* type;
        } params[] = {
            { "xyAbsolute", kOfxParamDoubleTypeXYAbsolute },
            { "normalisedXYAbsolute", kOfxParamDoubleTypeNormalisedXYAbsolute },
            { "xy", kOfxParamDoubleTypeXY },
            { "plain", 0 },
        };
        for (const auto& param : params) {
            stat = defineDouble2D(paramSet, param.name, param.type);
            if (stat != kOfxStatOK) {
                return stat;
            }
        }

        return kOfxStatOK;
    }
    if ( (std::strcmp(action, kOfxActionCreateInstance) == 0) || (std::strcmp(action, kOfxActionDestroyInstance) == 0) ||
         (std::strcmp(action, kOfxImageEffectActionRender) == 0) ) {
        return kOfxStatOK;
    }

    return kOfxStatReplyDefault;
}

void
setHost(OfxHost* host)
{
    gHost = host;
}

OfxPlugin gPlugin = { kOfxImageEffectPluginApi, 1, "net.natron.test.HostOverlayHandle", 1, 0, setHost, pluginMain };
NatronTestPluginRegistrar gRegistrar(&gPlugin);
} // anonymous namespace
