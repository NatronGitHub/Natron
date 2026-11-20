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

// net.natron.test.OverlayInput (Tests/OfxOverlayInput_Test.cpp): a filter with an overlay interact that
// records the inputs of its pen and key actions. Counters ("OverlayInput.*"):
// - <action>.count, <action>.viewportX, <action>.viewportY, <action>.pressure1000 for the pen actions
//   (penDown, penMotion, penUp): the number of actions, and the sums of kOfxInteractPropPenViewportPosition
//   and of 1000 * kOfxInteractPropPenPressure;
// - <action>.string.<kOfxPropKeyString> for the key actions (keyDown, keyUp, keyRepeat).

#include <cstring>
#include <string>

#include <ofxCore.h>
#include <ofxImageEffect.h>
#include <ofxInteract.h>
#include <ofxKeySyms.h>
#include <ofxProperty.h>

#include "NatronTestPluginRegistry.h"

namespace {
OfxHost* gHost = 0;
const OfxPropertySuiteV1* gPropSuite = 0;
const OfxImageEffectSuiteV1* gEffectSuite = 0;

const std::string kPrefix("OverlayInput.");

void
recordPen(const char* name,
          OfxPropertySetHandle inArgs)
{
    int viewportPos[2] = { 0, 0 };
    double pressure = -1.;

    gPropSuite->propGetIntN(inArgs, kOfxInteractPropPenViewportPosition, 2, viewportPos);
    gPropSuite->propGetDouble(inArgs, kOfxInteractPropPenPressure, 0, &pressure);
    natronTestIncrementCounter(kPrefix + name + ".count");
    natronTestIncrementCounter(kPrefix + name + ".viewportX", viewportPos[0]);
    natronTestIncrementCounter(kPrefix + name + ".viewportY", viewportPos[1]);
    natronTestIncrementCounter( kPrefix + name + ".pressure1000", (int)(pressure * 1000. + 0.5) );
}

void
recordKey(const char* name,
          OfxPropertySetHandle inArgs)
{
    const char* keyString = 0;

    if ( (gPropSuite->propGetString(inArgs, kOfxPropKeyString, 0, &keyString) != kOfxStatOK) || !keyString ) {
        natronTestIncrementCounter(kPrefix + name + ".noString");

        return;
    }
    natronTestIncrementCounter(kPrefix + name + ".string." + keyString);
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
        recordPen("penDown", inArgs);
    } else if (std::strcmp(action, kOfxInteractActionPenMotion) == 0) {
        recordPen("penMotion", inArgs);
    } else if (std::strcmp(action, kOfxInteractActionPenUp) == 0) {
        recordPen("penUp", inArgs);
    } else if (std::strcmp(action, kOfxInteractActionKeyDown) == 0) {
        recordKey("keyDown", inArgs);
    } else if (std::strcmp(action, kOfxInteractActionKeyUp) == 0) {
        recordKey("keyUp", inArgs);
    } else if (std::strcmp(action, kOfxInteractActionKeyRepeat) == 0) {
        recordKey("keyRepeat", inArgs);
    }

    return kOfxStatReplyDefault;
}

OfxStatus
overlayInputPluginMain(const char* action,
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

        return (gPropSuite && gEffectSuite) ? kOfxStatOK : kOfxStatErrMissingHostFeature;
    }
    if (std::strcmp(action, kOfxActionDescribe) == 0) {
        OfxPropertySetHandle props;
        OfxStatus stat = gEffectSuite->getPropertySet(effect, &props);
        if (stat != kOfxStatOK) {
            return stat;
        }
        gPropSuite->propSetString(props, kOfxPropLabel, 0, "OverlayInputTest");
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
    kOfxImageEffectPluginApi, 1, "net.natron.test.OverlayInput", 1, 0, setHost, overlayInputPluginMain
};
NatronTestPluginRegistrar gRegistrar(&gPlugin);
} // anonymous namespace
