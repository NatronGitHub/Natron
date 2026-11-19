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

// net.natron.test.Bytes (Tests/OfxBytesParam_Test.cpp): OFX 1.5 Bytes params (kOfxParamTypeBytes).
// Params:
// - bytes: a Bytes param whose default holds NUL bytes (kNatronTestBytesDefault);
// - bytesNoDefault: a Bytes param without a default;
// - bytesAnimates: a Bytes param with kOfxParamPropAnimates = 1;
// - trigger: an Integer param. Its instanceChanged sets "bytes": 1: 256 bytes 0..255, 2: a NULL OfxBytes,
//   3: a zero length; then reads it back. 4: sets the default of "bytes" to "dyn" from a buffer on the stack.
// The render action reads "bytes" and prints its length and checksum on stderr (for NatronRenderer runs).
// Counters ("Bytes.*"): defined / defineFailed (paramDefine of the Bytes params), instanceChanged,
// readBackOK / readBackFailed (the value read back in instanceChanged), render, renderLength and
// renderChecksum (the sum of the bytes) of the last render, renderFailed.

#include <cstdio>
#include <cstring>
#include <mutex>
#include <vector>

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

// the default of "bytes" (also in Tests/OfxBytesParam_Test.cpp): it must stay valid while instances are
// created, so it is static
const unsigned char gDefaultData[] = { 'N', 0, 'a', 0xff, 0 };
const OfxBytes gDefault = { gDefaultData, sizeof(gDefaultData) };

std::mutex gRenderMutex;
int gLastRenderLength = 0;
int gLastRenderChecksum = 0;

OfxStatus
describe(OfxImageEffectHandle effect)
{
    OfxPropertySetHandle props;
    OfxStatus stat = gEffectSuite->getPropertySet(effect, &props);

    if (stat != kOfxStatOK) {
        return stat;
    }
    gPropSuite->propSetString(props, kOfxPropLabel, 0, "BytesTest");
    gPropSuite->propSetString(props, kOfxImageEffectPluginPropGrouping, 0, "Other/Test");
    gPropSuite->propSetString(props, kOfxImageEffectPropSupportedContexts, 0, kOfxImageEffectContextFilter);
    gPropSuite->propSetString(props, kOfxImageEffectPropSupportedPixelDepths, 0, kOfxBitDepthFloat);
    gPropSuite->propSetString(props, kOfxImageEffectPluginRenderThreadSafety, 0, kOfxImageEffectRenderFullySafe);

    return kOfxStatOK;
}

// define a Bytes param, counting the result of paramDefine
OfxPropertySetHandle
defineBytes(OfxParamSetHandle paramSet,
            const char* name)
{
    OfxPropertySetHandle props = 0;
    OfxStatus stat = gParamSuite->paramDefine(paramSet, kOfxParamTypeBytes, name, &props);

    natronTestIncrementCounter( (stat == kOfxStatOK && props) ? "Bytes.defined" : "Bytes.defineFailed" );

    return stat == kOfxStatOK ? props : 0;
}

OfxStatus
describeInContext(OfxImageEffectHandle effect)
{
    const char* clips[] = { kOfxImageEffectSimpleSourceClipName, kOfxImageEffectOutputClipName };

    for (int i = 0; i < 2; ++i) {
        OfxPropertySetHandle props;
        OfxStatus stat = gEffectSuite->clipDefine(effect, clips[i], &props);
        if (stat != kOfxStatOK) {
            return stat;
        }
        gPropSuite->propSetString(props, kOfxImageEffectPropSupportedComponents, 0, kOfxImageComponentRGBA);
    }

    OfxParamSetHandle paramSet;
    OfxStatus stat = gEffectSuite->getParamSet(effect, &paramSet);
    if (stat != kOfxStatOK) {
        return stat;
    }
    OfxPropertySetHandle props = defineBytes(paramSet, "bytes");
    if (props) {
        gPropSuite->propSetPointer(props, kOfxParamPropDefault, 0, (void*)&gDefault);
    }
    defineBytes(paramSet, "bytesNoDefault");
    props = defineBytes(paramSet, "bytesAnimates");
    if (props) {
        gPropSuite->propSetInt(props, kOfxParamPropAnimates, 0, 1);
    }
    gParamSuite->paramDefine(paramSet, kOfxParamTypeInteger, "trigger", &props);

    return kOfxStatOK;
}

OfxStatus
instanceChanged(OfxImageEffectHandle effect,
                OfxPropertySetHandle inArgs)
{
    const char* name = 0;

    if ( !inArgs || (gPropSuite->propGetString(inArgs, kOfxPropName, 0, &name) != kOfxStatOK) ||
         (std::strcmp(name, "trigger") != 0) ) {
        return kOfxStatReplyDefault;
    }
    natronTestIncrementCounter("Bytes.instanceChanged");
    OfxParamSetHandle paramSet;
    OfxParamHandle trigger, bytes;
    int mode = 0;
    if ( (gEffectSuite->getParamSet(effect, &paramSet) != kOfxStatOK) ||
         (gParamSuite->paramGetHandle(paramSet, "trigger", &trigger, 0) != kOfxStatOK) ||
         (gParamSuite->paramGetHandle(paramSet, "bytes", &bytes, 0) != kOfxStatOK) ||
         (gParamSuite->paramGetValue(trigger, &mode) != kOfxStatOK) ) {
        return kOfxStatFailed;
    }
    std::vector<unsigned char> expected;
    OfxStatus stat = kOfxStatOK;
    if (mode == 1) {
        for (int i = 0; i < 256; ++i) {
            expected.push_back( (unsigned char)i );
        }
        // the host copies the data: it is freed when this function returns
        OfxBytes value = { &expected[0], expected.size() };
        stat = gParamSuite->paramSetValue(bytes, &value);
    } else if (mode == 2) {
        stat = gParamSuite->paramSetValue(bytes, (const OfxBytes*)0);
    } else if (mode == 3) {
        OfxBytes value = { gDefaultData, 0 };
        stat = gParamSuite->paramSetValue(bytes, &value);
    } else if (mode == 4) {
        // a new default, from data that is only valid during the call
        OfxPropertySetHandle props;
        if (gParamSuite->paramGetPropertySet(bytes, &props) != kOfxStatOK) {
            return kOfxStatFailed;
        }
        unsigned char buffer[3] = { 'd', 'y', 'n' };
        OfxBytes def = { buffer, sizeof(buffer) };
        gPropSuite->propSetPointer(props, kOfxParamPropDefault, 0, (void*)&def);
        std::memset(buffer, 0x55, sizeof(buffer));

        return kOfxStatOK;
    } else {
        return kOfxStatOK;
    }
    OfxBytes value = { 0, 12345 };
    bool ok = (stat == kOfxStatOK) && (gParamSuite->paramGetValue(bytes, &value) == kOfxStatOK) &&
              (value.length == expected.size()) &&
              ( expected.empty() ? (value.data == 0) : (value.data && std::memcmp(value.data, &expected[0], expected.size()) == 0) );
    natronTestIncrementCounter(ok ? "Bytes.readBackOK" : "Bytes.readBackFailed");

    return kOfxStatOK;
}

// a fixed region of definition, so that the node renders without an input
OfxStatus
getRegionOfDefinition(OfxPropertySetHandle outArgs)
{
    double rod[4] = { 0., 0., 16., 16. };

    gPropSuite->propSetDoubleN(outArgs, kOfxImageEffectPropRegionOfDefinition, 4, rod);

    return kOfxStatOK;
}

// read "bytes" during the render (the tests don't look at the images)
OfxStatus
render(OfxImageEffectHandle effect)
{
    OfxParamSetHandle paramSet;
    OfxParamHandle bytes;
    OfxBytes value = { 0, 0 };

    if ( (gEffectSuite->getParamSet(effect, &paramSet) != kOfxStatOK) ||
         (gParamSuite->paramGetHandle(paramSet, "bytes", &bytes, 0) != kOfxStatOK) ||
         (gParamSuite->paramGetValue(bytes, &value) != kOfxStatOK) ||
         ( (value.length > 0) && !value.data ) ) {
        natronTestIncrementCounter("Bytes.renderFailed");

        return kOfxStatFailed;
    }
    int checksum = 0;
    for (size_t i = 0; i < value.length; ++i) {
        checksum += value.data[i];
    }
    {
        // renderLength and renderChecksum are the values of the last render (the counters are sums)
        std::lock_guard<std::mutex> l(gRenderMutex);
        natronTestIncrementCounter( "Bytes.renderLength", (int)value.length - gLastRenderLength );
        natronTestIncrementCounter( "Bytes.renderChecksum", checksum - gLastRenderChecksum );
        gLastRenderLength = (int)value.length;
        gLastRenderChecksum = checksum;
    }
    natronTestIncrementCounter("Bytes.render");
    std::fprintf(stderr, "net.natron.test.Bytes render: %d bytes, checksum %d\n", (int)value.length, checksum);

    return kOfxStatOK;
}

OfxStatus
bytesPluginMain(const char* action,
                const void* handle,
                OfxPropertySetHandle inArgs,
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
    if (std::strcmp(action, kOfxActionInstanceChanged) == 0) {
        return instanceChanged(effect, inArgs);
    }
    if (std::strcmp(action, kOfxImageEffectActionGetRegionOfDefinition) == 0) {
        return getRegionOfDefinition(outArgs);
    }
    if (std::strcmp(action, kOfxImageEffectActionRender) == 0) {
        return render(effect);
    }

    return kOfxStatReplyDefault;
}

void
setHost(OfxHost* host)
{
    gHost = host;
}

OfxPlugin gPlugin = {
    kOfxImageEffectPluginApi, 1, "net.natron.test.Bytes", 1, 0, setHost, bytesPluginMain
};
NatronTestPluginRegistrar gRegistrar(&gPlugin);
} // anonymous namespace
