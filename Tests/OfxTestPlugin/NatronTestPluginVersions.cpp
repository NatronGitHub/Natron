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

// A reader and a writer, each in two major versions, 90.0 and 91.0 (Tests/ReadWriteVersion_Test.cpp, #1681):
// a project loads the version of the decoder or encoder of its Read and Write nodes that it was saved with,
// not the newest one. They read and write nothing.
// The Read and Write nodes only wrap the bundled readers and writers (ReadNode::isBundledReader(),
// WriteNode::isBundledWriter()), so they use the IDs of ReadCDR (openfx-arena) and WritePFM (openfx-io),
// with majors above those of the real plug-ins, and only the "natrontestversion" file extension.
// Warning: in the processes that load this bundle (the Tests binary and the GUI tests, see
// Tests/CMakeLists.txt), these are the newest ReadCDR and WritePFM, which read and write nothing: a
// test that reads .cdr or writes .pfm files through a Read or Write node must use another format.

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
versionPluginMain(bool isReader,
                  const char* action,
                  OfxImageEffectHandle effect)
{
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
        gPropSuite->propSetString(props, kOfxPropLabel, 0, isReader ? "ReadVersionTest" : "WriteVersionTest");
        gPropSuite->propSetString(props, kOfxImageEffectPluginPropGrouping, 0, "Other/Test");
        gPropSuite->propSetString(props, kOfxImageEffectPropSupportedContexts, 0,
                                  isReader ? kOfxImageEffectContextReader : kOfxImageEffectContextWriter);
        gPropSuite->propSetString(props, kOfxImageEffectPropSupportedPixelDepths, 0, kOfxBitDepthFloat);
        gPropSuite->propSetString(props, kOfxImageEffectPluginRenderThreadSafety, 0, kOfxImageEffectRenderFullySafe);
        gPropSuite->propSetString(props, kTuttleOfxImageEffectPropSupportedExtensions, 0, "natrontestversion");
        gPropSuite->propSetDouble(props, kTuttleOfxImageEffectPropEvaluation, 0, 50.);

        return kOfxStatOK;
    }
    if (std::strcmp(action, kOfxImageEffectActionDescribeInContext) == 0) {
        OfxPropertySetHandle props;
        // A Source clip in both, as the bundled readers and writers (openfx-io's GenericReader and
        // GenericWriter) define it: the Read node's only input ("Sync") is the Source clip of its decoder
        // (ReadNode::addAcceptedComponents() asks the decoder for input 0), optional for a reader.
        OfxStatus stat = gEffectSuite->clipDefine(effect, kOfxImageEffectSimpleSourceClipName, &props);
        if (stat != kOfxStatOK) {
            return stat;
        }
        gPropSuite->propSetString(props, kOfxImageEffectPropSupportedComponents, 0, kOfxImageComponentRGBA);
        if (isReader) {
            gPropSuite->propSetInt(props, kOfxImageClipPropOptional, 0, 1);
        }
        stat = gEffectSuite->clipDefine(effect, kOfxImageEffectOutputClipName, &props);
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
        gPropSuite->propSetInt(props, kOfxParamPropStringFilePathExists, 0, isReader ? 1 : 0);

        return kOfxStatOK;
    }
    if (std::strcmp(action, kOfxImageEffectActionRender) == 0) {
        return kOfxStatOK;
    }

    return kOfxStatReplyDefault;
}

OfxStatus
readerMain(const char* action,
           const void* handle,
           OfxPropertySetHandle /*inArgs*/,
           OfxPropertySetHandle /*outArgs*/)
{
    return versionPluginMain(true, action, (OfxImageEffectHandle)handle);
}

OfxStatus
writerMain(const char* action,
           const void* handle,
           OfxPropertySetHandle /*inArgs*/,
           OfxPropertySetHandle /*outArgs*/)
{
    return versionPluginMain(false, action, (OfxImageEffectHandle)handle);
}

void
setHost(OfxHost* host)
{
    gHost = host;
}

// the IDs of PLUGINID_OFX_READCDR and PLUGINID_OFX_WRITEPFM (Engine/EffectInstance.h)
OfxPlugin gPlugins[] = {
    { kOfxImageEffectPluginApi, 1, "fr.inria.openfx.ReadCDR", 90, 0, setHost, readerMain },
    { kOfxImageEffectPluginApi, 1, "fr.inria.openfx.ReadCDR", 91, 0, setHost, readerMain },
    { kOfxImageEffectPluginApi, 1, "fr.inria.openfx.WritePFM", 90, 0, setHost, writerMain },
    { kOfxImageEffectPluginApi, 1, "fr.inria.openfx.WritePFM", 91, 0, setHost, writerMain },
};
NatronTestPluginRegistrar gRegistrars[] = {
    NatronTestPluginRegistrar(&gPlugins[0]), NatronTestPluginRegistrar(&gPlugins[1]),
    NatronTestPluginRegistrar(&gPlugins[2]), NatronTestPluginRegistrar(&gPlugins[3]),
};
} // anonymous namespace
