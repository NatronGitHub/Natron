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

// net.natron.test.ClipComponentsView (Tests/OfxClipComponentsView_Test.cpp): a multi-planar filter
// (kFnOfxImageEffectPropMultiPlanar, nuke/fnOfxExtensions.h) whose kFnOfxImageEffectActionGetClipComponents
// counts the view it gets in kFnOfxImageEffectPropView ("ClipComponentsView.view<N>", N < 0 if it is
// missing) and, as the Support library's plug-ins do (setPassThroughClip(clip, args.time, args.view)),
// returns it as the pass-through view. It asks for the color plane of both clips. Its render action does
// not write the output image.

#include <cstring>
#include <string>

#include <ofxCore.h>
#include <ofxImageEffect.h>
#include <ofxProperty.h>
#include <nuke/fnOfxExtensions.h>

#include "NatronTestPluginRegistry.h"

namespace {
OfxHost* gHost = 0;
const OfxPropertySuiteV1* gPropSuite = 0;
const OfxImageEffectSuiteV1* gEffectSuite = 0;

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

OfxStatus
clipComponentsViewMain(const char* action,
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
        gPropSuite->propSetString(props, kOfxPropLabel, 0, "ClipComponentsView");
        gPropSuite->propSetString(props, kOfxImageEffectPluginPropGrouping, 0, "Other/Test");
        gPropSuite->propSetString(props, kOfxImageEffectPropSupportedContexts, 0, kOfxImageEffectContextFilter);
        gPropSuite->propSetString(props, kOfxImageEffectPropSupportedPixelDepths, 0, kOfxBitDepthFloat);
        gPropSuite->propSetString(props, kOfxImageEffectPluginRenderThreadSafety, 0, kOfxImageEffectRenderFullySafe);
        gPropSuite->propSetInt(props, kFnOfxImageEffectPropMultiPlanar, 0, 1);

        return kOfxStatOK;
    }
    if (std::strcmp(action, kOfxImageEffectActionDescribeInContext) == 0) {
        OfxStatus stat = defineClip(effect, kOfxImageEffectSimpleSourceClipName);
        if (stat != kOfxStatOK) {
            return stat;
        }

        return defineClip(effect, kOfxImageEffectOutputClipName);
    }
    if (std::strcmp(action, kFnOfxImageEffectActionGetClipComponents) == 0) {
        int view = -1;
        if (gPropSuite->propGetInt(inArgs, kFnOfxImageEffectPropView, 0, &view) != kOfxStatOK) {
            view = -1;
        }
        natronTestIncrementCounter( "ClipComponentsView.view" + std::to_string(view) );
        gPropSuite->propSetString(outArgs, kFnOfxImageEffectActionGetClipComponentsPropString kOfxImageEffectOutputClipName, 0, kFnOfxImagePlaneColour);
        gPropSuite->propSetString(outArgs, kFnOfxImageEffectActionGetClipComponentsPropString kOfxImageEffectSimpleSourceClipName, 0, kFnOfxImagePlaneColour);
        gPropSuite->propSetString(outArgs, kFnOfxImageEffectPropPassThroughClip, 0, kOfxImageEffectSimpleSourceClipName);
        gPropSuite->propSetInt(outArgs, kFnOfxImageEffectPropPassThroughView, 0, view);

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

OfxPlugin gPlugin = { kOfxImageEffectPluginApi, 1, "net.natron.test.ClipComponentsView", 1, 0, setHost, clipComponentsViewMain };
NatronTestPluginRegistrar gRegistrar(&gPlugin);
} // anonymous namespace
