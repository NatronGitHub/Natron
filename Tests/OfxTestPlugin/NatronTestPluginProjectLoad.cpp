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

// net.natron.test.ProjectLoad (Tests/OfxProjectLoad_Test.cpp): after a project load, Natron calls instanceChanged on
// the input clips of an instance "after kOfxActionCreateInstance and after the host has hooked up all clips, with
// property kOfxPropType = kOfxTypeClip", and clipGetRegionOfDefinition must work in that action (openfx main, after
// 1.5.1, from #184/#191; Natron uses kOfxChangePluginEdited, see kProjectLoadChangeReason in
// Engine/OfxEffectInstance.cpp).
//
// Like a writer (GenericWriter, openfx-io), the plug-in sets its "premult" and "frameRate" parameters from its Source
// clip when the user connects it (kOfxChangeUserEdited), and keeps them otherwise: a host that sent
// kOfxChangeUserEdited after a project load would overwrite the values saved in the project. "file" is never set by
// the plug-in. Like AppendClip (openfx-misc) with its read-only "lastFrame", if "setDerived" is on, the plug-in sets
// "derived" to the height of the Source RoD on any clip change of Source, and getClipPreferences gives the output that
// frame rate: a value changed in the action after the project load must update the metadata. Like WriteFFmpeg with its
// info parameters, if "setInfo" is on, the plug-in sets "info", which is neither saved (kOfxParamPropPersistent = 0)
// nor rendered (kOfxParamPropEvaluateOnChange = 0), to the width of the Source RoD on any clip change of Source.
// Clips: Source, Mask (optional), Output. Counters ("ProjectLoad.*"):
// - created: the instances created;
// - begin.<reason>, end.<reason>: the kOfxActionBegin/EndInstanceChanged actions;
// - clip.<clip>.<reason>: the kOfxActionInstanceChanged actions with kOfxPropType = kOfxTypeClip;
// - param.<reason>: the kOfxActionInstanceChanged actions with kOfxPropType = kOfxTypeParameter;
// - connected: the clip instanceChanged actions where the clip is connected (kOfxImageClipPropConnected);
// - rod: the clip instanceChanged actions where clipGetRegionOfDefinition gives a non-empty region;
// - sourcePAR100: the sum of 100 times the pixel aspect ratio of Source in those actions (the clip preferences);
// - sourceFrameRate: the sum of the frame rates of Source in those actions (the metadata of the input);
// - error.outsideBracket: an instanceChanged that is not between Begin and End;
// - error.reasonMismatch: an instanceChanged or an End whose reason is not the one of the Begin.

#include <cstring>
#include <map>
#include <mutex>
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

std::mutex gMutex;
std::map<OfxImageEffectHandle, std::string> gBrackets; // reason of the open bracket of each instance

const char* const kClipMask = "Mask";

std::string
getString(OfxPropertySetHandle props,
          const char* name)
{
    const char* value = 0;

    if ( (gPropSuite->propGetString(props, name, 0, &value) != kOfxStatOK) || !value ) {
        return std::string();
    }

    return value;
}

OfxParamHandle
getParam(OfxImageEffectHandle effect,
         const char* name)
{
    OfxParamSetHandle paramSet = 0;
    OfxParamHandle param = 0;

    if ( (gEffectSuite->getParamSet(effect, &paramSet) != kOfxStatOK) ||
         (gParamSuite->paramGetHandle(paramSet, name, &param, 0) != kOfxStatOK) ) {
        return 0;
    }

    return param;
}

OfxStatus
describe(OfxImageEffectHandle effect)
{
    OfxPropertySetHandle props;
    OfxStatus stat = gEffectSuite->getPropertySet(effect, &props);

    if (stat != kOfxStatOK) {
        return stat;
    }
    gPropSuite->propSetString(props, kOfxPropLabel, 0, "ProjectLoadTest");
    gPropSuite->propSetString(props, kOfxImageEffectPluginPropGrouping, 0, "Other/Test");
    gPropSuite->propSetString(props, kOfxImageEffectPropSupportedContexts, 0, kOfxImageEffectContextGeneral);
    gPropSuite->propSetString(props, kOfxImageEffectPropSupportedPixelDepths, 0, kOfxBitDepthFloat);
    gPropSuite->propSetString(props, kOfxImageEffectPluginRenderThreadSafety, 0, kOfxImageEffectRenderFullySafe);

    return kOfxStatOK;
}

OfxStatus
describeInContext(OfxImageEffectHandle effect)
{
    const char* clips[] = { kOfxImageEffectSimpleSourceClipName, kClipMask, kOfxImageEffectOutputClipName };

    for (int i = 0; i < 3; ++i) {
        OfxPropertySetHandle props;
        OfxStatus stat = gEffectSuite->clipDefine(effect, clips[i], &props);
        if (stat != kOfxStatOK) {
            return stat;
        }
        gPropSuite->propSetString(props, kOfxImageEffectPropSupportedComponents, 0, kOfxImageComponentRGBA);
        if (std::strcmp(clips[i], kClipMask) == 0) {
            gPropSuite->propSetInt(props, kOfxImageClipPropOptional, 0, 1);
        }
    }

    OfxParamSetHandle paramSet;
    if (gEffectSuite->getParamSet(effect, &paramSet) != kOfxStatOK) {
        return kOfxStatFailed;
    }
    OfxPropertySetHandle props;
    if (gParamSuite->paramDefine(paramSet, kOfxParamTypeBoolean, "premult", &props) != kOfxStatOK) {
        return kOfxStatFailed;
    }
    gPropSuite->propSetInt(props, kOfxParamPropDefault, 0, 0);
    if (gParamSuite->paramDefine(paramSet, kOfxParamTypeDouble, "frameRate", &props) != kOfxStatOK) {
        return kOfxStatFailed;
    }
    gPropSuite->propSetDouble(props, kOfxParamPropDefault, 0, 24.);
    if (gParamSuite->paramDefine(paramSet, kOfxParamTypeString, "file", &props) != kOfxStatOK) {
        return kOfxStatFailed;
    }
    gPropSuite->propSetString(props, kOfxParamPropDefault, 0, "");
    if (gParamSuite->paramDefine(paramSet, kOfxParamTypeBoolean, "setDerived", &props) != kOfxStatOK) {
        return kOfxStatFailed;
    }
    gPropSuite->propSetInt(props, kOfxParamPropDefault, 0, 0);
    if (gParamSuite->paramDefine(paramSet, kOfxParamTypeDouble, "derived", &props) != kOfxStatOK) {
        return kOfxStatFailed;
    }
    gPropSuite->propSetDouble(props, kOfxParamPropDefault, 0, 0.);
    if (gParamSuite->paramDefine(paramSet, kOfxParamTypeBoolean, "setInfo", &props) != kOfxStatOK) {
        return kOfxStatFailed;
    }
    gPropSuite->propSetInt(props, kOfxParamPropDefault, 0, 0);
    if (gParamSuite->paramDefine(paramSet, kOfxParamTypeDouble, "info", &props) != kOfxStatOK) {
        return kOfxStatFailed;
    }
    gPropSuite->propSetDouble(props, kOfxParamPropDefault, 0, 0.);
    gPropSuite->propSetInt(props, kOfxParamPropPersistent, 0, 0);
    gPropSuite->propSetInt(props, kOfxParamPropEvaluateOnChange, 0, 0);
    gPropSuite->propSetInt(props, kOfxParamPropAnimates, 0, 0);

    return kOfxStatOK;
}

bool
getSetDerived(OfxImageEffectHandle effect,
              double* derived)
{
    OfxParamHandle setDerived = getParam(effect, "setDerived");
    OfxParamHandle derivedParam = getParam(effect, "derived");
    int on = 0;

    if ( !setDerived || !derivedParam || (gParamSuite->paramGetValue(setDerived, &on) != kOfxStatOK) || !on ) {
        return false;
    }
    if (derived) {
        gParamSuite->paramGetValue(derivedParam, derived);
    }

    return true;
}

void
clipChanged(OfxImageEffectHandle effect,
            const std::string& clipName,
            const std::string& reason,
            double time)
{
    natronTestIncrementCounter("ProjectLoad.clip." + clipName + '.' + reason);

    OfxImageClipHandle clip = 0;
    OfxPropertySetHandle clipProps = 0;
    if ( (gEffectSuite->clipGetHandle(effect, clipName.c_str(), &clip, &clipProps) != kOfxStatOK) || !clip ) {
        return;
    }
    int connected = 0;
    gPropSuite->propGetInt(clipProps, kOfxImageClipPropConnected, 0, &connected);
    if (!connected) {
        return;
    }
    natronTestIncrementCounter("ProjectLoad.connected");
    double par = 0.;
    gPropSuite->propGetDouble(clipProps, kOfxImagePropPixelAspectRatio, 0, &par);
    natronTestIncrementCounter( "ProjectLoad.sourcePAR100", (int)(par * 100. + 0.5) );
    double frameRate = 0.;
    gPropSuite->propGetDouble(clipProps, kOfxImageEffectPropFrameRate, 0, &frameRate);
    natronTestIncrementCounter( "ProjectLoad.sourceFrameRate", (int)(frameRate + 0.5) );
    OfxRectD rod = { 0., 0., 0., 0. };
    const bool hasRoD = (gEffectSuite->clipGetRegionOfDefinition(clip, time, &rod) == kOfxStatOK) && (rod.x2 > rod.x1) && (rod.y2 > rod.y1);
    if (hasRoD) {
        natronTestIncrementCounter("ProjectLoad.rod");
    }

    // on any change of the source: the value derived from it
    if ( hasRoD && (clipName == kOfxImageEffectSimpleSourceClipName) && getSetDerived(effect, 0) ) {
        gParamSuite->paramSetValue(getParam(effect, "derived"), rod.y2 - rod.y1);
    }

    // on any change of the source: a display-only value derived from it
    if ( hasRoD && (clipName == kOfxImageEffectSimpleSourceClipName) ) {
        OfxParamHandle setInfo = getParam(effect, "setInfo");
        int on = 0;
        if ( setInfo && (gParamSuite->paramGetValue(setInfo, &on) == kOfxStatOK) && on ) {
            gParamSuite->paramSetValue(getParam(effect, "info"), rod.x2 - rod.x1);
        }
    }

    // the user connected the source: set the parameters from it, as a writer does
    if ( (clipName == kOfxImageEffectSimpleSourceClipName) && (reason == kOfxChangeUserEdited) ) {
        OfxParamHandle premult = getParam(effect, "premult");
        OfxParamHandle frameRate = getParam(effect, "frameRate");
        if (premult) {
            gParamSuite->paramSetValue(premult, 1);
        }
        if (frameRate) {
            gParamSuite->paramSetValue(frameRate, 50.);
        }
    }
}

OfxStatus
projectLoadPluginMain(const char* action,
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
    if (std::strcmp(action, kOfxImageEffectActionGetClipPreferences) == 0) {
        double derived = 0.;
        if ( !getSetDerived(effect, &derived) || (derived <= 0.) ) {
            return kOfxStatReplyDefault;
        }
        gPropSuite->propSetDouble(outArgs, kOfxImageEffectPropFrameRate, 0, derived);

        return kOfxStatOK;
    }
    if (std::strcmp(action, kOfxActionCreateInstance) == 0) {
        natronTestIncrementCounter("ProjectLoad.created");

        return kOfxStatOK;
    }
    if (std::strcmp(action, kOfxActionDestroyInstance) == 0) {
        std::lock_guard<std::mutex> l(gMutex);
        gBrackets.erase(effect);

        return kOfxStatOK;
    }
    if (std::strcmp(action, kOfxActionBeginInstanceChanged) == 0) {
        const std::string reason = getString(inArgs, kOfxPropChangeReason);
        {
            std::lock_guard<std::mutex> l(gMutex);
            gBrackets[effect] = reason;
        }
        natronTestIncrementCounter("ProjectLoad.begin." + reason);

        return kOfxStatOK;
    }
    if (std::strcmp(action, kOfxActionEndInstanceChanged) == 0) {
        const std::string reason = getString(inArgs, kOfxPropChangeReason);
        bool mismatch;
        {
            std::lock_guard<std::mutex> l(gMutex);
            std::map<OfxImageEffectHandle, std::string>::iterator found = gBrackets.find(effect);
            mismatch = ( found == gBrackets.end() ) || (found->second != reason);
            if ( found != gBrackets.end() ) {
                gBrackets.erase(found);
            }
        }
        if (mismatch) {
            natronTestIncrementCounter("ProjectLoad.error.reasonMismatch");
        }
        natronTestIncrementCounter("ProjectLoad.end." + reason);

        return kOfxStatOK;
    }
    if (std::strcmp(action, kOfxActionInstanceChanged) == 0) {
        const std::string type = getString(inArgs, kOfxPropType);
        const std::string name = getString(inArgs, kOfxPropName);
        const std::string reason = getString(inArgs, kOfxPropChangeReason);
        double time = 0.;
        gPropSuite->propGetDouble(inArgs, kOfxPropTime, 0, &time);
        {
            std::lock_guard<std::mutex> l(gMutex);
            std::map<OfxImageEffectHandle, std::string>::iterator found = gBrackets.find(effect);
            if ( found == gBrackets.end() ) {
                natronTestIncrementCounter("ProjectLoad.error.outsideBracket");
            } else if (found->second != reason) {
                natronTestIncrementCounter("ProjectLoad.error.reasonMismatch");
            }
        }
        if (type == kOfxTypeClip) {
            clipChanged(effect, name, reason, time);
        } else if (type == kOfxTypeParameter) {
            natronTestIncrementCounter("ProjectLoad.param." + reason);
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

OfxPlugin gPlugin = {
    kOfxImageEffectPluginApi, 1, "net.natron.test.ProjectLoad", 1, 0, setHost, projectLoadPluginMain
};
NatronTestPluginRegistrar gRegistrar(&gPlugin);
} // anonymous namespace
