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

// OpenGL render plug-ins (Tests/OfxOpenGLSupport_Test.cpp): generators whose
// kOfxActionOpenGLContextAttached returns kOfxStatFailed ("the host should not attempt to run the
// plug-in in OpenGL render mode", ofxGPURender.h):
// - net.natron.test.GLAttachFailed: kOfxImageEffectPropOpenGLRenderSupported "true";
// - net.natron.test.GLAttachFailedNeeded: kOfxImageEffectPropOpenGLRenderSupported "needed".
// Counters ("<name>.*"): attach, detach, glRenders, cpuRenders. The renders don't write any pixel.

#include <cstring>
#include <string>

#include <ofxCore.h>
#include <ofxGPURender.h>
#include <ofxImageEffect.h>
#include <ofxProperty.h>

#include "NatronTestPluginRegistry.h"

namespace {
OfxHost* gHost = 0;
const OfxPropertySuiteV1* gPropSuite = 0;
const OfxImageEffectSuiteV1* gEffectSuite = 0;

OfxStatus
glPluginMain(const char* name,
             const char* openGLRenderSupported,
             const char* action,
             OfxImageEffectHandle effect,
             OfxPropertySetHandle inArgs)
{
    const std::string prefix(name);

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
        gPropSuite->propSetString(props, kOfxPropLabel, 0, name);
        gPropSuite->propSetString(props, kOfxImageEffectPluginPropGrouping, 0, "Other/Test");
        gPropSuite->propSetString(props, kOfxImageEffectPropSupportedContexts, 0, kOfxImageEffectContextGenerator);
        gPropSuite->propSetString(props, kOfxImageEffectPropSupportedPixelDepths, 0, kOfxBitDepthFloat);
        gPropSuite->propSetString(props, kOfxImageEffectPluginRenderThreadSafety, 0, kOfxImageEffectRenderFullySafe);
        gPropSuite->propSetString(props, kOfxImageEffectPropOpenGLRenderSupported, 0, openGLRenderSupported);

        return kOfxStatOK;
    }
    if (std::strcmp(action, kOfxImageEffectActionDescribeInContext) == 0) {
        OfxPropertySetHandle props;
        OfxStatus stat = gEffectSuite->clipDefine(effect, kOfxImageEffectOutputClipName, &props);
        if (stat != kOfxStatOK) {
            return stat;
        }
        gPropSuite->propSetString(props, kOfxImageEffectPropSupportedComponents, 0, kOfxImageComponentRGBA);

        return kOfxStatOK;
    }
    if (std::strcmp(action, kOfxActionOpenGLContextAttached) == 0) {
        natronTestIncrementCounter(prefix + ".attach");

        return kOfxStatFailed;
    }
    if (std::strcmp(action, kOfxActionOpenGLContextDetached) == 0) {
        natronTestIncrementCounter(prefix + ".detach");

        return kOfxStatOK;
    }
    if (std::strcmp(action, kOfxImageEffectActionRender) == 0) {
        int openGLEnabled = 0;
        if (inArgs) {
            gPropSuite->propGetInt(inArgs, kOfxImageEffectPropOpenGLEnabled, 0, &openGLEnabled);
        }
        natronTestIncrementCounter(prefix + (openGLEnabled ? ".glRenders" : ".cpuRenders"));

        return kOfxStatOK;
    }

    return kOfxStatReplyDefault;
}

OfxStatus
glAttachFailedMain(const char* action,
                   const void* handle,
                   OfxPropertySetHandle inArgs,
                   OfxPropertySetHandle /*outArgs*/)
{
    return glPluginMain("GLAttachFailed", "true", action, (OfxImageEffectHandle)handle, inArgs);
}

OfxStatus
glAttachFailedNeededMain(const char* action,
                         const void* handle,
                         OfxPropertySetHandle inArgs,
                         OfxPropertySetHandle /*outArgs*/)
{
    return glPluginMain("GLAttachFailedNeeded", "needed", action, (OfxImageEffectHandle)handle, inArgs);
}

void
setHost(OfxHost* host)
{
    gHost = host;
}

OfxPlugin gPlugins[] = {
    { kOfxImageEffectPluginApi, 1, "net.natron.test.GLAttachFailed", 1, 0, setHost, glAttachFailedMain },
    { kOfxImageEffectPluginApi, 1, "net.natron.test.GLAttachFailedNeeded", 1, 0, setHost, glAttachFailedNeededMain },
};
NatronTestPluginRegistrar gRegistrars[] = {
    NatronTestPluginRegistrar(&gPlugins[0]), NatronTestPluginRegistrar(&gPlugins[1]),
};
} // anonymous namespace
