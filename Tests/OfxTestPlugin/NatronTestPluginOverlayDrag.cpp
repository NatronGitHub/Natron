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

// net.natron.test.OverlayDrag (Tests/gui/test_overlay_drag_undo.py): a filter whose overlay interact captures
// every drag and, at each pen motion, sets its double parameters "y" then "x" in one edit block, as the Transform
// overlay sets all its parameters: y is the number of pen motions of the drag divided by 2, so that it changes only
// at every other motion, and x is the pen x. It is an identity: it never renders.

#include <cstring>

#include <ofxCore.h>
#include <ofxImageEffect.h>
#include <ofxInteract.h>
#include <ofxParam.h>
#include <ofxProperty.h>

#include "NatronTestPluginRegistry.h"

namespace {
OfxHost* gHost = 0;
const OfxPropertySuiteV1* gPropSuite = 0;
const OfxImageEffectSuiteV1* gEffectSuite = 0;
const OfxParameterSuiteV1* gParamSuite = 0;

// the current drag (one at a time: the main thread runs the overlays)
bool gDragging = false;
int gMotions = 0; // its pen motions

OfxStatus
setParams(OfxPropertySetHandle inArgs)
{
    OfxImageEffectHandle effect = 0;
    OfxParamSetHandle paramSet = 0;
    OfxParamHandle x = 0, y = 0;
    double pen[2] = { 0., 0. };

    if ( (gPropSuite->propGetPointer(inArgs, kOfxPropEffectInstance, 0, (void**)&effect) != kOfxStatOK) || !effect ||
         (gEffectSuite->getParamSet(effect, &paramSet) != kOfxStatOK) ||
         (gParamSuite->paramGetHandle(paramSet, "x", &x, 0) != kOfxStatOK) ||
         (gParamSuite->paramGetHandle(paramSet, "y", &y, 0) != kOfxStatOK) ||
         (gPropSuite->propGetDoubleN(inArgs, kOfxInteractPropPenPosition, 2, pen) != kOfxStatOK) ) {
        return kOfxStatFailed;
    }
    gParamSuite->paramEditBegin(paramSet, "drag");
    // y first: when it changes, it is the first change of the block, which starts a new undo command unless
    // it is merged into the previous one
    gParamSuite->paramSetValue(y, (double)(gMotions / 2));
    gParamSuite->paramSetValue(x, pen[0]);
    gParamSuite->paramEditEnd(paramSet);

    return kOfxStatOK;
}

OfxStatus
overlayMain(const char* action,
            const void* /*handle*/,
            OfxPropertySetHandle inArgs,
            OfxPropertySetHandle /*outArgs*/)
{
    if ( (std::strcmp(action, kOfxActionDescribe) == 0) || (std::strcmp(action, kOfxActionCreateInstance) == 0) ||
         (std::strcmp(action, kOfxActionDestroyInstance) == 0) ) {
        return kOfxStatOK;
    }
    if (std::strcmp(action, kOfxInteractActionPenDown) == 0) {
        gDragging = true;
        gMotions = 0;

        return kOfxStatOK; // captures the drag
    }
    if (std::strcmp(action, kOfxInteractActionPenMotion) == 0) {
        if (!gDragging) {
            return kOfxStatReplyDefault; // hovering
        }
        ++gMotions;

        return setParams(inArgs);
    }
    if (std::strcmp(action, kOfxInteractActionPenUp) == 0) {
        gDragging = false;

        return kOfxStatOK;
    }

    return kOfxStatReplyDefault;
}

OfxStatus
defineParams(OfxImageEffectHandle effect)
{
    OfxParamSetHandle paramSet = 0;

    if (gEffectSuite->getParamSet(effect, &paramSet) != kOfxStatOK) {
        return kOfxStatFailed;
    }
    const char* names[] = { "x", "y" };
    for (int i = 0; i < 2; ++i) {
        OfxPropertySetHandle props;
        if (gParamSuite->paramDefine(paramSet, kOfxParamTypeDouble, names[i], &props) != kOfxStatOK) {
            return kOfxStatFailed;
        }
        gPropSuite->propSetString(props, kOfxParamPropDoubleType, 0, kOfxParamDoubleTypePlain);
        gPropSuite->propSetInt(props, kOfxParamPropAnimates, 0, 0);
    }

    return kOfxStatOK;
}

OfxStatus
overlayDragPluginMain(const char* action,
                      const void* handle,
                      OfxPropertySetHandle /*inArgs*/,
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
        gPropSuite->propSetString(props, kOfxPropLabel, 0, "OverlayDragTest");
        gPropSuite->propSetString(props, kOfxImageEffectPluginPropGrouping, 0, "Other/Test");
        gPropSuite->propSetString(props, kOfxImageEffectPropSupportedContexts, 0, kOfxImageEffectContextFilter);
        gPropSuite->propSetString(props, kOfxImageEffectPropSupportedPixelDepths, 0, kOfxBitDepthFloat);
        gPropSuite->propSetString(props, kOfxImageEffectPluginRenderThreadSafety, 0, kOfxImageEffectRenderFullySafe);
        gPropSuite->propSetPointer(props, kOfxImageEffectPluginPropOverlayInteractV1, 0, (void*)&overlayMain);

        return kOfxStatOK;
    }
    if (std::strcmp(action, kOfxImageEffectActionDescribeInContext) == 0) {
        const char* clips[] = { kOfxImageEffectSimpleSourceClipName, kOfxImageEffectOutputClipName };
        for (int i = 0; i < 2; ++i) {
            OfxPropertySetHandle props;
            OfxStatus stat = gEffectSuite->clipDefine(effect, clips[i], &props);
            if (stat != kOfxStatOK) {
                return stat;
            }
            gPropSuite->propSetString(props, kOfxImageEffectPropSupportedComponents, 0, kOfxImageComponentRGBA);
        }

        return defineParams(effect);
    }
    if (std::strcmp(action, kOfxImageEffectActionIsIdentity) == 0) {
        // never renders: the output is the input
        gPropSuite->propSetString(outArgs, kOfxPropName, 0, kOfxImageEffectSimpleSourceClipName);

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
    kOfxImageEffectPluginApi, 1, "net.natron.test.OverlayDrag", 1, 0, setHost, overlayDragPluginMain
};
NatronTestPluginRegistrar gRegistrar(&gPlugin);
} // anonymous namespace
