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

// Plug-ins that set some of their labels or none (Tests/OfxPluginLabel_Test.cpp, #2023): generators that do
// nothing, written against the C API, which sets only the properties it names. HostSupport gives the three labels
// of an effect descriptor the plug-in identifier as their default (ofxCore.h, kOfxPropLabel: "Default - the
// kOfxPropName the object was created with"; openfx#384).
// - net.natron.test.LabelShortOnly sets only kOfxPropShortLabel ("ShortOnly");
// - net.natron.test.LabelLongOnly sets only kOfxPropLongLabel ("LongOnly");
// - net.natron.test.LabelNone sets no label.

#include <cstring>

#include <ofxCore.h>
#include <ofxImageEffect.h>
#include <ofxProperty.h>

#include "NatronTestPluginRegistry.h"

namespace {
OfxHost* gHost = nullptr;
const OfxPropertySuiteV1* gPropSuite = nullptr;
const OfxImageEffectSuiteV1* gEffectSuite = nullptr;

// labelProperty is the label property the plug-in sets to label, none if it is nullptr
OfxStatus
labelPluginMain(const char* labelProperty,
                const char* label,
                const char* action,
                OfxImageEffectHandle effect)
{
    if (std::strcmp(action, kOfxActionLoad) == 0) {
        if (!gHost) {
            return kOfxStatErrMissingHostFeature;
        }
        gPropSuite = static_cast<const OfxPropertySuiteV1*>( gHost->fetchSuite(gHost->host, kOfxPropertySuite, 1) );
        gEffectSuite = static_cast<const OfxImageEffectSuiteV1*>( gHost->fetchSuite(gHost->host, kOfxImageEffectSuite, 1) );

        return (gPropSuite && gEffectSuite) ? kOfxStatOK : kOfxStatErrMissingHostFeature;
    }
    if (std::strcmp(action, kOfxActionDescribe) == 0) {
        OfxPropertySetHandle props;
        OfxStatus stat = gEffectSuite->getPropertySet(effect, &props);
        if (stat != kOfxStatOK) {
            return stat;
        }
        if (labelProperty) {
            gPropSuite->propSetString(props, labelProperty, 0, label);
        }
        gPropSuite->propSetString(props, kOfxImageEffectPluginPropGrouping, 0, "Other/Test");
        gPropSuite->propSetString(props, kOfxImageEffectPropSupportedContexts, 0, kOfxImageEffectContextGenerator);
        gPropSuite->propSetString(props, kOfxImageEffectPropSupportedPixelDepths, 0, kOfxBitDepthFloat);
        gPropSuite->propSetString(props, kOfxImageEffectPluginRenderThreadSafety, 0, kOfxImageEffectRenderFullySafe);

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

// the effect handle of a plug-in's main entry point (the C API passes it as a const void*)
OfxImageEffectHandle
effectHandle(const void* handle)
{
    return static_cast<OfxImageEffectHandle>( const_cast<void*>(handle) );
}

OfxStatus
shortOnlyMain(const char* action,
              const void* handle,
              OfxPropertySetHandle /*inArgs*/,
              OfxPropertySetHandle /*outArgs*/)
{
    return labelPluginMain(kOfxPropShortLabel, "ShortOnly", action, effectHandle(handle));
}

OfxStatus
longOnlyMain(const char* action,
             const void* handle,
             OfxPropertySetHandle /*inArgs*/,
             OfxPropertySetHandle /*outArgs*/)
{
    return labelPluginMain(kOfxPropLongLabel, "LongOnly", action, effectHandle(handle));
}

OfxStatus
noLabelMain(const char* action,
            const void* handle,
            OfxPropertySetHandle /*inArgs*/,
            OfxPropertySetHandle /*outArgs*/)
{
    return labelPluginMain(nullptr, nullptr, action, effectHandle(handle));
}

void
setHost(OfxHost* host)
{
    gHost = host;
}

OfxPlugin gPlugins[] = {
    { kOfxImageEffectPluginApi, 1, "net.natron.test.LabelShortOnly", 1, 0, setHost, shortOnlyMain },
    { kOfxImageEffectPluginApi, 1, "net.natron.test.LabelLongOnly", 1, 0, setHost, longOnlyMain },
    { kOfxImageEffectPluginApi, 1, "net.natron.test.LabelNone", 1, 0, setHost, noLabelMain },
};
NatronTestPluginRegistrar gRegistrars[] = {
    NatronTestPluginRegistrar(&gPlugins[0]), NatronTestPluginRegistrar(&gPlugins[1]), NatronTestPluginRegistrar(&gPlugins[2]),
};
} // anonymous namespace
