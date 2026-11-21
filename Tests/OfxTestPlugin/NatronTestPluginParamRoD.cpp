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

// net.natron.test.ParamRoD (Tests/ExpressionDepsRenderArgs_Test.cpp): a generator whose region of definition is
// (0, 0, size, size), where size is its "size" parameter at the time of the action. It counts:
// - "ParamRoD.rod.<size>": the size read by kOfxImageEffectActionGetRegionOfDefinition;
// - "ParamRoD.render.<size>": the size read by kOfxImageEffectActionRender;
// - "ParamRoD.renderWidth.<width>": the width of the render window.
// Its render action does not write the output image.

#include <cstring>
#include <sstream>
#include <string>

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

void
count(const char* name,
      double value)
{
    std::ostringstream ss;

    ss << "ParamRoD." << name << '.' << (int)(value + 0.5);
    natronTestIncrementCounter( ss.str() );
}

bool
getSize(OfxImageEffectHandle effect,
        OfxPropertySetHandle inArgs,
        double* size)
{
    OfxParamSetHandle paramSet = 0;
    OfxParamHandle param = 0;
    double time = 0.;

    return (gEffectSuite->getParamSet(effect, &paramSet) == kOfxStatOK) &&
           (gParamSuite->paramGetHandle(paramSet, "size", &param, 0) == kOfxStatOK) &&
           (gPropSuite->propGetDouble(inArgs, kOfxPropTime, 0, &time) == kOfxStatOK) &&
           (gParamSuite->paramGetValueAtTime(param, time, size) == kOfxStatOK);
}

OfxStatus
paramRoDMain(const char* action,
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
        OfxPropertySetHandle props;
        OfxStatus stat = gEffectSuite->getPropertySet(effect, &props);
        if (stat != kOfxStatOK) {
            return stat;
        }
        gPropSuite->propSetString(props, kOfxPropLabel, 0, "ParamRoD");
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
        OfxParamSetHandle paramSet = 0;
        if ( (gEffectSuite->getParamSet(effect, &paramSet) != kOfxStatOK) ||
             (gParamSuite->paramDefine(paramSet, kOfxParamTypeDouble, "size", &props) != kOfxStatOK) ) {
            return kOfxStatFailed;
        }
        gPropSuite->propSetDouble(props, kOfxParamPropDefault, 0, 16.);

        return kOfxStatOK;
    }
    if (std::strcmp(action, kOfxImageEffectActionGetRegionOfDefinition) == 0) {
        double size = 0.;
        if ( !getSize(effect, inArgs, &size) ) {
            return kOfxStatFailed;
        }
        count("rod", size);
        double rod[4] = { 0., 0., size, size };
        gPropSuite->propSetDoubleN(outArgs, kOfxImageEffectPropRegionOfDefinition, 4, rod);

        return kOfxStatOK;
    }
    if (std::strcmp(action, kOfxImageEffectActionRender) == 0) {
        double size = 0.;
        if ( getSize(effect, inArgs, &size) ) {
            count("render", size);
        }
        int window[4] = { 0, 0, 0, 0 };
        if (gPropSuite->propGetIntN(inArgs, kOfxImageEffectPropRenderWindow, 4, window) == kOfxStatOK) {
            count("renderWidth", window[2] - window[0]);
        }

        return kOfxStatOK;
    }

    return kOfxStatReplyDefault;
}

void
setHost(OfxHost* host)
{
    gHost = host;
}

OfxPlugin gPlugin = { kOfxImageEffectPluginApi, 1, "net.natron.test.ParamRoD", 1, 0, setHost, paramRoDMain };
NatronTestPluginRegistrar gRegistrar(&gPlugin);
} // anonymous namespace
