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

// A reader with no Source clip, only the Output clip that the reader context requires
// (Tests/ReadNodeInputs_Test.cpp, #1826). The Read node has one input, which it hands to its decoder as
// input 0, but a decoder doesn't have to define a Source clip (openfx-io's GenericReader does).
// The Read node only wraps the bundled readers (ReadNode::isBundledReader()), so this one uses the ID of
// ReadPDF (openfx-arena), with a major above that of the real plug-in, and only the
// "natrontestnosource" file extension. Warning: in the processes that load this bundle (the Tests
// binary and the GUI tests), this is the newest ReadPDF, which reads nothing.

#include <cstring>

#include <ofxCore.h>
#include <ofxImageEffect.h>
#include <ofxParam.h>
#include <ofxProperty.h>
#include <tuttle/ofxReadWrite.h>

#include "NatronTestPluginRegistry.h"

namespace {
OfxHost* gHost = 0;
const OfxPropertySuiteV1* gPropSuite = 0;
const OfxImageEffectSuiteV1* gEffectSuite = 0;
const OfxParameterSuiteV1* gParamSuite = 0;

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
        gPropSuite->propSetString(props, kOfxPropLabel, 0, "ReadNoSourceTest");
        gPropSuite->propSetString(props, kOfxImageEffectPluginPropGrouping, 0, "Other/Test");
        gPropSuite->propSetString(props, kOfxImageEffectPropSupportedContexts, 0, kOfxImageEffectContextReader);
        gPropSuite->propSetString(props, kOfxImageEffectPropSupportedPixelDepths, 0, kOfxBitDepthFloat);
        gPropSuite->propSetString(props, kOfxImageEffectPluginRenderThreadSafety, 0, kOfxImageEffectRenderFullySafe);
        gPropSuite->propSetString(props, kTuttleOfxImageEffectPropSupportedExtensions, 0, "natrontestnosource");
        gPropSuite->propSetDouble(props, kTuttleOfxImageEffectPropEvaluation, 0, 50.);

        return kOfxStatOK;
    }
    if (std::strcmp(action, kOfxImageEffectActionDescribeInContext) == 0) {
        OfxPropertySetHandle props;
        // no Source clip
        OfxStatus stat = gEffectSuite->clipDefine(effect, kOfxImageEffectOutputClipName, &props);
        if (stat != kOfxStatOK) {
            return stat;
        }
        gPropSuite->propSetString(props, kOfxImageEffectPropSupportedComponents, 0, kOfxImageComponentRGBA);

        OfxParamSetHandle paramSet;
        if (gEffectSuite->getParamSet(effect, &paramSet) != kOfxStatOK) {
            return kOfxStatFailed;
        }
        if (gParamSuite->paramDefine(paramSet, kOfxParamTypeString, kOfxImageEffectFileParamName, &props) != kOfxStatOK) {
            return kOfxStatFailed;
        }
        gPropSuite->propSetString(props, kOfxParamPropStringMode, 0, kOfxParamStringIsFilePath);
        gPropSuite->propSetInt(props, kOfxParamPropStringFilePathExists, 0, 1);

        return kOfxStatOK;
    }
    if (std::strcmp(action, kOfxImageEffectActionRender) == 0) {
        return kOfxStatOK;
    }

    return kOfxStatReplyDefault;
}

void
setHost(OfxHost* host)
{
    gHost = host;
}

// the ID of PLUGINID_OFX_READPDF (Engine/EffectInstance.h)
OfxPlugin gPlugin = { kOfxImageEffectPluginApi, 1, "fr.inria.openfx.ReadPDF", 90, 0, setHost, pluginMain };
NatronTestPluginRegistrar gRegistrar(&gPlugin);
} // anonymous namespace
