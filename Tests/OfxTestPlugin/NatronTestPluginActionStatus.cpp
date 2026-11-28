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

// net.natron.test.ActionStatus (Tests/OfxSuiteStatus_Test.cpp): a filter whose actions return the status
// that a test chose, to check how Natron handles each status an OFX action may return. The status of an
// action is the counter "ActionStatus.status.<action>" (0, the default, is kOfxStatOK), set by the test
// with NatronTestPlugin::setCounter():
// - "render": kOfxImageEffectActionRender. It counts its calls ("ActionStatus.render"). If the counter
//   "ActionStatus.render.waitForAbort" is set, it first counts "ActionStatus.render.waiting" and waits
//   (at most 10 s) until abort() returns 1, as a plug-in that stops a render the host aborted does, and
//   counts "ActionStatus.render.aborted" if it did. It doesn't write the output image.
// - "rodScaleOne" and "rodOtherScale": kOfxImageEffectActionGetRegionOfDefinition at a render scale of 1,
//   and at another scale. It counts its calls ("ActionStatus.rod.scaleOne", "ActionStatus.rod.otherScale").
//   When the status is kOfxStatOK, it sets the region of definition: (0, 0, kSize, kSize) with kSize 8 at
//   scale 1, and (0, 0, 2 kSize, 2 kSize) at other scales, so that a test can tell which call's RoD the host
//   uses. That is deliberately not conformant (the RoD is in canonical coordinates, the same at any scale).
//   With another status, it doesn't set it.
// - "transform": kFnOfxImageEffectActionGetTransform (nuke/fnOfxExtensions.h; the plug-in sets
//   kFnOfxImageEffectCanTransform). When the status is kOfxStatOK, it sets the clip name to Source and the
//   matrix to kTransform (a scale by 2), and otherwise sets neither.

#include <chrono>
#include <cstring>
#include <string>
#include <thread>

#include <ofxCore.h>
#include <ofxImageEffect.h>
#include <ofxProperty.h>
#include <nuke/fnOfxExtensions.h>

#include "NatronTestPluginRegistry.h"

namespace {
const double kSize = 8.;
const double kTransform[9] = { 2., 0., 0., 0., 2., 0., 0., 0., 1. };

OfxHost* gHost = 0;
const OfxPropertySuiteV1* gPropSuite = 0;
const OfxImageEffectSuiteV1* gEffectSuite = 0;

// the status the test chose for action ("ActionStatus.status.<action>")
OfxStatus
chosenStatus(const char* action)
{
    return (OfxStatus)natronTestGetCounter( std::string("ActionStatus.status.") + action );
}

OfxStatus
defineClip(OfxImageEffectHandle effect,
           const char* name)
{
    OfxPropertySetHandle props;
    OfxStatus stat = gEffectSuite->clipDefine(effect, name, &props);

    if (stat != kOfxStatOK) {
        return stat;
    }
    gPropSuite->propSetString(props, kOfxImageEffectPropSupportedComponents, 0, kOfxImageComponentRGBA);

    return kOfxStatOK;
}

// wait until the host aborts the render, at most 10 s: true if it did
bool
waitForAbort(OfxImageEffectHandle effect)
{
    const std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now() + std::chrono::seconds(10);

    while (std::chrono::steady_clock::now() < end) {
        if (gEffectSuite->abort(effect) == 1) {
            return true;
        }
        std::this_thread::sleep_for( std::chrono::milliseconds(1) );
    }

    return false;
}

OfxStatus
render(OfxImageEffectHandle effect)
{
    natronTestIncrementCounter("ActionStatus.render");
    if (natronTestGetCounter("ActionStatus.render.waitForAbort") != 0) {
        natronTestIncrementCounter("ActionStatus.render.waiting");
        if ( waitForAbort(effect) ) {
            natronTestIncrementCounter("ActionStatus.render.aborted");
        }
    }

    return chosenStatus("render");
}

OfxStatus
getRegionOfDefinition(OfxPropertySetHandle inArgs,
                      OfxPropertySetHandle outArgs)
{
    double scale[2] = { 0., 0. };
    gPropSuite->propGetDoubleN(inArgs, kOfxImageEffectPropRenderScale, 2, scale);
    const bool scaleIsOne = (scale[0] == 1.) && (scale[1] == 1.);
    natronTestIncrementCounter(scaleIsOne ? "ActionStatus.rod.scaleOne" : "ActionStatus.rod.otherScale");
    const OfxStatus stat = chosenStatus(scaleIsOne ? "rodScaleOne" : "rodOtherScale");
    if (stat == kOfxStatOK) {
        const double size = scaleIsOne ? kSize : 2. * kSize;
        const double rod[4] = { 0., 0., size, size };
        gPropSuite->propSetDoubleN(outArgs, kOfxImageEffectPropRegionOfDefinition, 4, rod);
    }

    return stat;
}

OfxStatus
getTransform(OfxPropertySetHandle outArgs)
{
    const OfxStatus stat = chosenStatus("transform");

    if (stat == kOfxStatOK) {
        gPropSuite->propSetString(outArgs, kOfxPropName, 0, kOfxImageEffectSimpleSourceClipName);
        gPropSuite->propSetDoubleN(outArgs, kFnOfxPropMatrix2D, 9, kTransform);
    }

    return stat;
}

OfxStatus
actionStatusMain(const char* action,
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

        return (gPropSuite && gEffectSuite) ? kOfxStatOK : kOfxStatErrMissingHostFeature;
    }
    if (std::strcmp(action, kOfxActionDescribe) == 0) {
        OfxPropertySetHandle props;
        OfxStatus stat = gEffectSuite->getPropertySet(effect, &props);
        if (stat != kOfxStatOK) {
            return stat;
        }
        gPropSuite->propSetString(props, kOfxPropLabel, 0, "ActionStatus");
        gPropSuite->propSetString(props, kOfxImageEffectPluginPropGrouping, 0, "Other/Test");
        gPropSuite->propSetString(props, kOfxImageEffectPropSupportedContexts, 0, kOfxImageEffectContextFilter);
        gPropSuite->propSetString(props, kOfxImageEffectPropSupportedPixelDepths, 0, kOfxBitDepthFloat);
        // one render call per frame
        gPropSuite->propSetString(props, kOfxImageEffectPluginRenderThreadSafety, 0, kOfxImageEffectRenderInstanceSafe);
        gPropSuite->propSetInt(props, kOfxImageEffectPluginPropHostFrameThreading, 0, 0);
        gPropSuite->propSetInt(props, kOfxImageEffectPropSupportsTiles, 0, 0);
        gPropSuite->propSetInt(props, kFnOfxImageEffectCanTransform, 0, 1);

        return kOfxStatOK;
    }
    if (std::strcmp(action, kOfxImageEffectActionDescribeInContext) == 0) {
        OfxStatus stat = defineClip(effect, kOfxImageEffectSimpleSourceClipName);
        if (stat != kOfxStatOK) {
            return stat;
        }

        return defineClip(effect, kOfxImageEffectOutputClipName);
    }
    if (std::strcmp(action, kOfxImageEffectActionGetRegionOfDefinition) == 0) {
        return getRegionOfDefinition(inArgs, outArgs);
    }
    if (std::strcmp(action, kFnOfxImageEffectActionGetTransform) == 0) {
        return getTransform(outArgs);
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

OfxPlugin gPlugin = { kOfxImageEffectPluginApi, 1, "net.natron.test.ActionStatus", 1, 0, setHost, actionStatusMain };
NatronTestPluginRegistrar gRegistrar(&gPlugin);
} // anonymous namespace
