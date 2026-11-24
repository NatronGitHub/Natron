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

// net.natron.test.MaskIdentity (Tests/FailedRoDRender_Test.cpp, #1656): a filter with an optional mask input,
// "Mask", that only looks at the mask outside its render action as the masked plug-ins of openfx-misc do (e.g.
// Merge): kOfxImageEffectActionIsIdentity returns Source when the mask is connected and its region of definition
// doesn't intersect the render window. Its region of definition is the one of Source. It counts its renders
// ("MaskIdentity.render"); its render action does not write the output image.

#include <cstring>

#include <ofxCore.h>
#include <ofxImageEffect.h>
#include <ofxProperty.h>

#include "NatronTestPluginRegistry.h"

namespace {
OfxHost* gHost = 0;
const OfxPropertySuiteV1* gPropSuite = 0;
const OfxImageEffectSuiteV1* gEffectSuite = 0;

const char* const kMaskClipName = "Mask";

OfxImageClipHandle
getClip(OfxImageEffectHandle effect,
        const char* name)
{
    OfxImageClipHandle clip = 0;

    if (gEffectSuite->clipGetHandle(effect, name, &clip, 0) != kOfxStatOK) {
        return 0;
    }

    return clip;
}

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

// kOfxImageEffectActionIsIdentity: Source if the mask is connected and doesn't intersect the render window
OfxStatus
isIdentity(OfxImageEffectHandle effect,
           OfxPropertySetHandle inArgs,
           OfxPropertySetHandle outArgs)
{
    OfxImageClipHandle mask = getClip(effect, kMaskClipName);
    OfxPropertySetHandle maskProps;
    int connected = 0;

    if ( !mask || (gEffectSuite->clipGetPropertySet(mask, &maskProps) != kOfxStatOK) ||
         (gPropSuite->propGetInt(maskProps, kOfxImageClipPropConnected, 0, &connected) != kOfxStatOK) || !connected ) {
        return kOfxStatReplyDefault;
    }
    OfxTime time = 0.;
    double scale[2] = { 1., 1. };
    int window[4] = { 0, 0, 0, 0 };
    OfxRectD maskRoD;
    if ( (gPropSuite->propGetDouble(inArgs, kOfxPropTime, 0, &time) != kOfxStatOK) ||
         (gPropSuite->propGetDoubleN(inArgs, kOfxImageEffectPropRenderScale, 2, scale) != kOfxStatOK) ||
         (gPropSuite->propGetIntN(inArgs, kOfxImageEffectPropRenderWindow, 4, window) != kOfxStatOK) ||
         (gEffectSuite->clipGetRegionOfDefinition(mask, time, &maskRoD) != kOfxStatOK) ) {
        return kOfxStatFailed;
    }
    // the render window in canonical coordinates (pixel aspect ratio 1)
    const double x1 = window[0] / scale[0], y1 = window[1] / scale[1], x2 = window[2] / scale[0], y2 = window[3] / scale[1];
    const bool intersects = (maskRoD.x1 < maskRoD.x2) && (maskRoD.y1 < maskRoD.y2) &&
                            (maskRoD.x1 < x2) && (x1 < maskRoD.x2) && (maskRoD.y1 < y2) && (y1 < maskRoD.y2);
    if (intersects) {
        return kOfxStatReplyDefault;
    }
    gPropSuite->propSetString(outArgs, kOfxPropName, 0, kOfxImageEffectSimpleSourceClipName);

    return kOfxStatOK;
}

OfxStatus
maskIdentityMain(const char* action,
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
        gPropSuite->propSetString(props, kOfxPropLabel, 0, "MaskIdentity");
        gPropSuite->propSetString(props, kOfxImageEffectPluginPropGrouping, 0, "Other/Test");
        gPropSuite->propSetString(props, kOfxImageEffectPropSupportedContexts, 0, kOfxImageEffectContextGeneral);
        gPropSuite->propSetString(props, kOfxImageEffectPropSupportedPixelDepths, 0, kOfxBitDepthFloat);
        gPropSuite->propSetString(props, kOfxImageEffectPluginRenderThreadSafety, 0, kOfxImageEffectRenderFullySafe);

        return kOfxStatOK;
    }
    if (std::strcmp(action, kOfxImageEffectActionDescribeInContext) == 0) {
        OfxStatus stat = defineClip(effect, kOfxImageEffectSimpleSourceClipName, kOfxImageComponentRGBA, false);
        if (stat == kOfxStatOK) {
            stat = defineClip(effect, kMaskClipName, kOfxImageComponentAlpha, true);
        }
        if (stat == kOfxStatOK) {
            stat = defineClip(effect, kOfxImageEffectOutputClipName, kOfxImageComponentRGBA, false);
        }

        return stat;
    }
    if (std::strcmp(action, kOfxImageEffectActionGetRegionOfDefinition) == 0) {
        OfxImageClipHandle source = getClip(effect, kOfxImageEffectSimpleSourceClipName);
        OfxTime time = 0.;
        OfxRectD rod;
        if ( !source || (gPropSuite->propGetDouble(inArgs, kOfxPropTime, 0, &time) != kOfxStatOK) ||
             (gEffectSuite->clipGetRegionOfDefinition(source, time, &rod) != kOfxStatOK) ) {
            return kOfxStatFailed;
        }
        gPropSuite->propSetDoubleN(outArgs, kOfxImageEffectPropRegionOfDefinition, 4, &rod.x1);

        return kOfxStatOK;
    }
    if (std::strcmp(action, kOfxImageEffectActionIsIdentity) == 0) {
        return isIdentity(effect, inArgs, outArgs);
    }
    if (std::strcmp(action, kOfxImageEffectActionRender) == 0) {
        natronTestIncrementCounter("MaskIdentity.render");

        return kOfxStatOK;
    }

    return kOfxStatReplyDefault;
}

void
setHost(OfxHost* host)
{
    gHost = host;
}

OfxPlugin gPlugin = { kOfxImageEffectPluginApi, 1, "net.natron.test.MaskIdentity", 1, 0, setHost, maskIdentityMain };
NatronTestPluginRegistrar gRegistrar(&gPlugin);
} // anonymous namespace
