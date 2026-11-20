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

// Obsolete plug-ins (Tests/OfxPluginObsolete_Test.cpp, #1491): generators that do nothing.
// - net.natron.test.Obsolete sets kOfxImageEffectPluginPropObsolete to 1 (OFX, after 1.5.1): Natron
//   doesn't offer it for new nodes, but it still loads in existing projects, from Python and by ID;
// - net.natron.test.NatronDeprecated sets only the former Natron extension property
//   "NatronOfxImageEffectPropDeprecated" (kNatronOfxImageEffectPropDeprecated, removed by #1491):
//   Natron ignores it, so the plug-in is offered for new nodes. The name is spelled out because the
//   #define is removed from ofxNatron.h (NatronGitHub/openfx#198); once HostSupport no longer declares
//   the property, propSetInt fails and the plug-in ignores that.

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
obsoletePluginMain(const char* label,
                   const char* obsoleteProperty,
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
        gPropSuite->propSetString(props, kOfxPropLabel, 0, label);
        gPropSuite->propSetString(props, kOfxImageEffectPluginPropGrouping, 0, "Other/Test");
        gPropSuite->propSetString(props, kOfxImageEffectPropSupportedContexts, 0, kOfxImageEffectContextGenerator);
        gPropSuite->propSetString(props, kOfxImageEffectPropSupportedPixelDepths, 0, kOfxBitDepthFloat);
        gPropSuite->propSetString(props, kOfxImageEffectPluginRenderThreadSafety, 0, kOfxImageEffectRenderFullySafe);
        gPropSuite->propSetInt(props, obsoleteProperty, 0, 1);

        return kOfxStatOK;
    }
    if (std::strcmp(action, kOfxImageEffectActionDescribeInContext) == 0) {
        OfxPropertySetHandle props;
        OfxStatus stat = gEffectSuite->clipDefine(effect, kOfxImageEffectOutputClipName, &props);
        if (stat != kOfxStatOK) {
            return stat;
        }
        gPropSuite->propSetString(props, kOfxImageEffectPropSupportedComponents, 0, kOfxImageComponentRGBA);

        return kOfxStatOK;
    }
    if (std::strcmp(action, kOfxImageEffectActionRender) == 0) {
        return kOfxStatOK;
    }

    return kOfxStatReplyDefault;
}

OfxStatus
obsoleteMain(const char* action,
             const void* handle,
             OfxPropertySetHandle /*inArgs*/,
             OfxPropertySetHandle /*outArgs*/)
{
    return obsoletePluginMain("Obsolete", kOfxImageEffectPluginPropObsolete, action, (OfxImageEffectHandle)handle);
}

OfxStatus
natronDeprecatedMain(const char* action,
                     const void* handle,
                     OfxPropertySetHandle /*inArgs*/,
                     OfxPropertySetHandle /*outArgs*/)
{
    return obsoletePluginMain("NatronDeprecated", "NatronOfxImageEffectPropDeprecated", action, (OfxImageEffectHandle)handle);
}

void
setHost(OfxHost* host)
{
    gHost = host;
}

OfxPlugin gPlugins[] = {
    { kOfxImageEffectPluginApi, 1, "net.natron.test.Obsolete", 1, 0, setHost, obsoleteMain },
    { kOfxImageEffectPluginApi, 1, "net.natron.test.NatronDeprecated", 1, 0, setHost, natronDeprecatedMain },
};
NatronTestPluginRegistrar gRegistrars[] = {
    NatronTestPluginRegistrar(&gPlugins[0]), NatronTestPluginRegistrar(&gPlugins[1]),
};
} // anonymous namespace
