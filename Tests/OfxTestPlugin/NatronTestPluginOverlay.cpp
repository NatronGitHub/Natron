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

// net.natron.test.OverlayV1V2 and net.natron.test.OverlayV2 (Tests/OfxDrawSuite_Test.cpp): an effect
// overlay registered with both kOfxImageEffectPluginPropOverlayInteractV1 and V2, or only with V2. Each
// draw counts which entry was called ("<id>.drawV1", "<id>.drawV2"), the
// kNatronOfxInteractPropDrawSuiteShadows value of its inArgs ("<id>.shadowsOn", "<id>.shadowsOff",
// "<id>.shadowsMissing"), and, in V2 draws, the DrawSuite calls that succeeded ("<id>.drawSuiteOK": a
// line from (4, 4.5) to (20, 4.5) and the text "ab" at (4, 10)). The render does nothing.

#include <cstring>
#include <string>

#include <ofxCore.h>
#include <ofxDrawSuite.h>
#include <ofxImageEffect.h>
#include <ofxInteract.h>
#include <ofxNatron.h>
#include <ofxProperty.h>

#include "NatronTestPluginRegistry.h"

namespace {
OfxHost* gHost = 0;
const OfxPropertySuiteV1* gPropSuite = 0;
const OfxImageEffectSuiteV1* gEffectSuite = 0;
const OfxDrawSuiteV1* gDrawSuite = 0; // OFX 1.5

OfxStatus
onLoad()
{
    if (!gHost) {
        return kOfxStatErrMissingHostFeature;
    }
    gPropSuite = (const OfxPropertySuiteV1*)gHost->fetchSuite(gHost->host, kOfxPropertySuite, 1);
    gEffectSuite = (const OfxImageEffectSuiteV1*)gHost->fetchSuite(gHost->host, kOfxImageEffectSuite, 1);
    gDrawSuite = (const OfxDrawSuiteV1*)gHost->fetchSuite(gHost->host, kOfxDrawSuite, 1);

    return (gPropSuite && gEffectSuite) ? kOfxStatOK : kOfxStatErrMissingHostFeature;
}

OfxStatus
describe(OfxImageEffectHandle effect,
         const char* label)
{
    OfxPropertySetHandle props;
    OfxStatus stat = gEffectSuite->getPropertySet(effect, &props);

    if (stat != kOfxStatOK) {
        return stat;
    }
    gPropSuite->propSetString(props, kOfxPropLabel, 0, label);
    gPropSuite->propSetString(props, kOfxImageEffectPluginPropGrouping, 0, "Other/Test");
    gPropSuite->propSetString(props, kOfxImageEffectPropSupportedContexts, 0, kOfxImageEffectContextFilter);
    gPropSuite->propSetString(props, kOfxImageEffectPropSupportedPixelDepths, 0, kOfxBitDepthFloat);
    gPropSuite->propSetString(props, kOfxImageEffectPluginRenderThreadSafety, 0, kOfxImageEffectRenderFullySafe);

    return kOfxStatOK;
}

OfxStatus
defineClips(OfxImageEffectHandle effect)
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

    return kOfxStatOK;
}

OfxStatus
commonAction(const char* action)
{
    if (std::strcmp(action, kOfxActionLoad) == 0) {
        return onLoad();
    }
    if ( (std::strcmp(action, kOfxActionCreateInstance) == 0) || (std::strcmp(action, kOfxActionDestroyInstance) == 0) ||
         (std::strcmp(action, kOfxImageEffectActionRender) == 0) ) {
        return kOfxStatOK;
    }

    return kOfxStatReplyDefault;
}

////////////////////////////////////////////////////////////////////////////////
// net.natron.test.OverlayV1V2 and net.natron.test.OverlayV2

OfxStatus
overlayDraw(const std::string& id,
            bool v2,
            OfxPropertySetHandle inArgs)
{
    natronTestIncrementCounter(id + (v2 ? ".drawV2" : ".drawV1"));
    int shadows = 0;
    if (gPropSuite->propGetInt(inArgs, kNatronOfxInteractPropDrawSuiteShadows, 0, &shadows) != kOfxStatOK) {
        natronTestIncrementCounter(id + ".shadowsMissing");
    } else {
        natronTestIncrementCounter(id + (shadows ? ".shadowsOn" : ".shadowsOff"));
    }
    if (v2) {
        // "If this is called via kOfxImageEffectPluginPropOverlayInteractV2, drawing MUST use DrawSuite"
        void* context = 0;
        if ( !gDrawSuite || (gPropSuite->propGetPointer(inArgs, kOfxInteractPropDrawContext, 0, &context) != kOfxStatOK) || !context ) {
            return kOfxStatFailed;
        }
        OfxDrawContextHandle dc = (OfxDrawContextHandle)context;
        const OfxPointD line[2] = { { 4., 4.5 }, { 20., 4.5 } };
        if (gDrawSuite->draw(dc, kOfxDrawPrimitiveLines, line, 2) == kOfxStatOK) {
            natronTestIncrementCounter(id + ".drawSuiteOK");
        }
        const OfxPointD pos = { 4., 10. };
        if (gDrawSuite->drawText(dc, "ab", &pos, kOfxDrawTextAlignmentLeft) == kOfxStatOK) {
            natronTestIncrementCounter(id + ".drawSuiteOK");
        }
    }

    return kOfxStatOK;
}

OfxStatus
overlayAction(const std::string& id,
              bool v2,
              const char* action,
              OfxPropertySetHandle inArgs)
{
    if ( (std::strcmp(action, kOfxActionDescribe) == 0) || (std::strcmp(action, kOfxActionCreateInstance) == 0) ||
         (std::strcmp(action, kOfxActionDestroyInstance) == 0) ) {
        return kOfxStatOK;
    }
    if (std::strcmp(action, kOfxInteractActionDraw) == 0) {
        return overlayDraw(id, v2, inArgs);
    }

    return kOfxStatReplyDefault;
}

OfxStatus
overlayV1V2MainV1(const char* action, const void* /*handle*/, OfxPropertySetHandle inArgs, OfxPropertySetHandle /*outArgs*/)
{
    return overlayAction("OverlayV1V2", false, action, inArgs);
}

OfxStatus
overlayV1V2MainV2(const char* action, const void* /*handle*/, OfxPropertySetHandle inArgs, OfxPropertySetHandle /*outArgs*/)
{
    return overlayAction("OverlayV1V2", true, action, inArgs);
}

OfxStatus
overlayV2MainV2(const char* action, const void* /*handle*/, OfxPropertySetHandle inArgs, OfxPropertySetHandle /*outArgs*/)
{
    return overlayAction("OverlayV2", true, action, inArgs);
}

OfxStatus
overlayPluginMain(const std::string& id,
                  bool hasV1,
                  const char* action,
                  const void* handle)
{
    OfxImageEffectHandle effect = (OfxImageEffectHandle)handle;

    if (std::strcmp(action, kOfxActionDescribe) == 0) {
        OfxStatus stat = describe(effect, (id + "Test").c_str());
        if (stat != kOfxStatOK) {
            return stat;
        }
        OfxPropertySetHandle props;
        stat = gEffectSuite->getPropertySet(effect, &props);
        if (stat != kOfxStatOK) {
            return stat;
        }
        if (hasV1) {
            gPropSuite->propSetPointer(props, kOfxImageEffectPluginPropOverlayInteractV1, 0, (void*)&overlayV1V2MainV1);
            gPropSuite->propSetPointer(props, kOfxImageEffectPluginPropOverlayInteractV2, 0, (void*)&overlayV1V2MainV2);
        } else {
            gPropSuite->propSetPointer(props, kOfxImageEffectPluginPropOverlayInteractV2, 0, (void*)&overlayV2MainV2);
        }

        return kOfxStatOK;
    }
    if (std::strcmp(action, kOfxImageEffectActionDescribeInContext) == 0) {
        return defineClips(effect);
    }

    return commonAction(action);
}

OfxStatus
overlayV1V2PluginMain(const char* action, const void* handle, OfxPropertySetHandle /*inArgs*/, OfxPropertySetHandle /*outArgs*/)
{
    return overlayPluginMain("OverlayV1V2", true, action, handle);
}

OfxStatus
overlayV2PluginMain(const char* action, const void* handle, OfxPropertySetHandle /*inArgs*/, OfxPropertySetHandle /*outArgs*/)
{
    return overlayPluginMain("OverlayV2", false, action, handle);
}

void
setHost(OfxHost* host)
{
    gHost = host;
}

OfxPlugin gOverlayV1V2Plugin = {
    kOfxImageEffectPluginApi, 1, "net.natron.test.OverlayV1V2", 1, 0, setHost, overlayV1V2PluginMain
};
OfxPlugin gOverlayV2Plugin = {
    kOfxImageEffectPluginApi, 1, "net.natron.test.OverlayV2", 1, 0, setHost, overlayV2PluginMain
};
NatronTestPluginRegistrar gOverlayV1V2Registrar(&gOverlayV1V2Plugin);
NatronTestPluginRegistrar gOverlayV2Registrar(&gOverlayV2Plugin);
} // anonymous namespace
