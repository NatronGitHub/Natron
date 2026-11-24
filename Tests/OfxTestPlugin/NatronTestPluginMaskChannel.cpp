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

// The mask channel menu of a node (Tests/MaskChannel_Test.cpp, #1685). They render nothing.
// - net.natron.test.AlphaGenerator: a generator whose output is alpha-only;
// - net.natron.test.Masked: a filter (Source, Output) with an optional Mask input (kOfxImageClipPropIsMask).

#include <cstring>

#include <ofxCore.h>
#include <ofxImageEffect.h>
#include <ofxProperty.h>

#include "NatronTestPluginRegistry.h"

namespace {
OfxHost* gHost = 0;
const OfxPropertySuiteV1* gPropSuite = 0;
const OfxImageEffectSuiteV1* gEffectSuite = 0;

OfxStatus
defineClip(OfxImageEffectHandle effect,
           const char* name,
           const char* components,
           bool isMask)
{
    OfxPropertySetHandle props;
    OfxStatus stat = gEffectSuite->clipDefine(effect, name, &props);

    if (stat != kOfxStatOK) {
        return stat;
    }
    gPropSuite->propSetString(props, kOfxImageEffectPropSupportedComponents, 0, components);
    if (isMask) {
        gPropSuite->propSetInt(props, kOfxImageClipPropOptional, 0, 1);
        gPropSuite->propSetInt(props, kOfxImageClipPropIsMask, 0, 1);
    }

    return kOfxStatOK;
}

OfxStatus
maskChannelPluginMain(bool isGenerator,
                      const char* action,
                      OfxImageEffectHandle effect)
{
    if (std::strcmp(action, kOfxActionLoad) == 0) {
        if (!gHost) {
            return kOfxStatErrMissingHostFeature;
        }
        gPropSuite = (const OfxPropertySuiteV1*)gHost->fetchSuite(gHost->host, kOfxPropertySuite, 1);
        gEffectSuite = (const OfxImageEffectSuiteV1*)gHost->fetchSuite(gHost->host, kOfxImageEffectSuite, 1);

        return (gPropSuite && gEffectSuite) ? kOfxStatOK : kOfxStatErrMissingHostFeature;
    }
    if (std::strcmp(action, kOfxActionDescribe) == 0) {
        OfxPropertySetHandle props;
        OfxStatus stat = gEffectSuite->getPropertySet(effect, &props);
        if (stat != kOfxStatOK) {
            return stat;
        }
        gPropSuite->propSetString(props, kOfxPropLabel, 0, isGenerator ? "AlphaGeneratorTest" : "MaskedTest");
        gPropSuite->propSetString(props, kOfxImageEffectPluginPropGrouping, 0, "Other/Test");
        gPropSuite->propSetString(props, kOfxImageEffectPropSupportedContexts, 0,
                                  isGenerator ? kOfxImageEffectContextGenerator : kOfxImageEffectContextGeneral);
        gPropSuite->propSetString(props, kOfxImageEffectPropSupportedPixelDepths, 0, kOfxBitDepthFloat);
        gPropSuite->propSetString(props, kOfxImageEffectPluginRenderThreadSafety, 0, kOfxImageEffectRenderFullySafe);

        return kOfxStatOK;
    }
    if (std::strcmp(action, kOfxImageEffectActionDescribeInContext) == 0) {
        if (isGenerator) {
            return defineClip(effect, kOfxImageEffectOutputClipName, kOfxImageComponentAlpha, false);
        }
        OfxStatus stat = defineClip(effect, kOfxImageEffectSimpleSourceClipName, kOfxImageComponentRGBA, false);
        if (stat == kOfxStatOK) {
            stat = defineClip(effect, "Mask", kOfxImageComponentAlpha, true);
        }
        if (stat == kOfxStatOK) {
            stat = defineClip(effect, kOfxImageEffectOutputClipName, kOfxImageComponentRGBA, false);
        }

        return stat;
    }
    if (std::strcmp(action, kOfxImageEffectActionRender) == 0) {
        return kOfxStatOK;
    }

    return kOfxStatReplyDefault;
}

OfxStatus
alphaGeneratorMain(const char* action,
                   const void* handle,
                   OfxPropertySetHandle /*inArgs*/,
                   OfxPropertySetHandle /*outArgs*/)
{
    return maskChannelPluginMain(true, action, (OfxImageEffectHandle)handle);
}

OfxStatus
maskedMain(const char* action,
           const void* handle,
           OfxPropertySetHandle /*inArgs*/,
           OfxPropertySetHandle /*outArgs*/)
{
    return maskChannelPluginMain(false, action, (OfxImageEffectHandle)handle);
}

void
setHost(OfxHost* host)
{
    gHost = host;
}

OfxPlugin gPlugins[] = {
    { kOfxImageEffectPluginApi, 1, "net.natron.test.AlphaGenerator", 1, 0, setHost, alphaGeneratorMain },
    { kOfxImageEffectPluginApi, 1, "net.natron.test.Masked", 1, 0, setHost, maskedMain },
};
NatronTestPluginRegistrar gRegistrars[] = {
    NatronTestPluginRegistrar(&gPlugins[0]), NatronTestPluginRegistrar(&gPlugins[1]),
};
} // anonymous namespace
