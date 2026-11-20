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

// net.natron.test.SequenceArgs (Tests/OfxSequenceRenderArgs_Test.cpp): a generator that checks the inArgs of
// kOfxImageEffectActionBeginSequenceRender and kOfxImageEffectActionEndSequenceRender (ofxImageEffect.h).
// It counts:
// - "SequenceArgs.begin" and "SequenceArgs.end": the actions;
// - "SequenceArgs.badStep": a kOfxImageEffectPropFrameStep that is not positive ("can be any positive
//   value");
// - "SequenceArgs.mismatch": an End whose frame range, frame step, kOfxPropIsInteractive or
//   kOfxImageEffectPropInteractiveRenderStatus differ from those of the last Begin of the same instance
//   (End is called "after a sequence of renders" with the same arguments);
// - "SequenceArgs.interactive": a Begin with kOfxPropIsInteractive set.
// Its render action does not write the output image.

#include <cstring>
#include <map>
#include <mutex>
#include <string>

#include <ofxCore.h>
#include <ofxImageEffect.h>
#include <ofxProperty.h>

#include "NatronTestPluginRegistry.h"

namespace {
OfxHost* gHost = 0;
const OfxPropertySuiteV1* gPropSuite = 0;
const OfxImageEffectSuiteV1* gEffectSuite = 0;

struct SequenceArgs
{
    double range[2];
    double step;
    int interactive;
    int interactiveRenderStatus;
};

std::mutex gBeginsMutex;
std::map<const void*, SequenceArgs> gBegins; // the last Begin of each instance

bool
getSequenceArgs(OfxPropertySetHandle inArgs,
                SequenceArgs* args)
{
    return inArgs &&
           (gPropSuite->propGetDoubleN(inArgs, kOfxImageEffectPropFrameRange, 2, args->range) == kOfxStatOK) &&
           (gPropSuite->propGetDouble(inArgs, kOfxImageEffectPropFrameStep, 0, &args->step) == kOfxStatOK) &&
           (gPropSuite->propGetInt(inArgs, kOfxPropIsInteractive, 0, &args->interactive) == kOfxStatOK) &&
           (gPropSuite->propGetInt(inArgs, kOfxImageEffectPropInteractiveRenderStatus, 0, &args->interactiveRenderStatus) == kOfxStatOK);
}

OfxStatus
sequenceArgsMain(const char* action,
                 const void* handle,
                 OfxPropertySetHandle inArgs,
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
        gPropSuite->propSetString(props, kOfxPropLabel, 0, "SequenceArgs");
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

        return kOfxStatOK;
    }
    if (std::strcmp(action, kOfxImageEffectActionBeginSequenceRender) == 0) {
        natronTestIncrementCounter("SequenceArgs.begin");
        SequenceArgs args;
        if ( !getSequenceArgs(inArgs, &args) ) {
            return kOfxStatFailed;
        }
        if ( !(args.step > 0.) ) {
            natronTestIncrementCounter("SequenceArgs.badStep");
        }
        if (args.interactive) {
            natronTestIncrementCounter("SequenceArgs.interactive");
        }
        std::lock_guard<std::mutex> l(gBeginsMutex);
        gBegins[handle] = args;

        return kOfxStatOK;
    }
    if (std::strcmp(action, kOfxImageEffectActionEndSequenceRender) == 0) {
        natronTestIncrementCounter("SequenceArgs.end");
        SequenceArgs args;
        if ( !getSequenceArgs(inArgs, &args) ) {
            return kOfxStatFailed;
        }
        if ( !(args.step > 0.) ) {
            natronTestIncrementCounter("SequenceArgs.badStep");
        }
        std::lock_guard<std::mutex> l(gBeginsMutex);
        std::map<const void*, SequenceArgs>::const_iterator found = gBegins.find(handle);
        if ( ( found == gBegins.end() ) ||
             (found->second.range[0] != args.range[0]) || (found->second.range[1] != args.range[1]) ||
             (found->second.step != args.step) || (found->second.interactive != args.interactive) ||
             (found->second.interactiveRenderStatus != args.interactiveRenderStatus) ) {
            natronTestIncrementCounter("SequenceArgs.mismatch");
        }

        return kOfxStatOK;
    }
    if (std::strcmp(action, kOfxActionDestroyInstance) == 0) {
        std::lock_guard<std::mutex> l(gBeginsMutex);
        gBegins.erase(handle);

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

OfxPlugin gPlugin = { kOfxImageEffectPluginApi, 1, "net.natron.test.SequenceArgs", 1, 0, setHost, sequenceArgsMain };
NatronTestPluginRegistrar gRegistrar(&gPlugin);
} // anonymous namespace
