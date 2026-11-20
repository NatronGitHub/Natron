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

// net.natron.test.InstanceEdit (Tests/OfxInstanceEdit_Test.cpp): checks the order of
// kOfxActionBeginInstanceEdit, kOfxActionEndInstanceEdit and kOfxActionDestroyInstance (ofxCore.h:
// End has "\pre kOfxActionBeginInstanceEdit has been called on the instance handle", and Begin
// "\post kOfxActionEndInstanceEdit will be called when the last editor is closed"), and the value of
// kOfxPropIsInteractive in these actions. Counters ("InstanceEdit.*"):
// - begin, end, created, destroyed: the actions received;
// - error.endWithoutBegin, error.beginTwice, error.destroyWhileEditing: the order errors;
// - interactiveInBegin, interactiveInEnd: the sum of kOfxPropIsInteractive in Begin and in End.

#include <cstring>
#include <mutex>
#include <set>

#include <ofxCore.h>
#include <ofxImageEffect.h>
#include <ofxProperty.h>

#include "NatronTestPluginRegistry.h"

namespace {
OfxHost* gHost = 0;
const OfxPropertySuiteV1* gPropSuite = 0;
const OfxImageEffectSuiteV1* gEffectSuite = 0;

std::mutex gMutex;
std::set<OfxImageEffectHandle> gEditing; // instances between Begin and End

int
isInteractive(OfxImageEffectHandle effect)
{
    OfxPropertySetHandle props;
    int value = -1;

    if ( (gEffectSuite->getPropertySet(effect, &props) != kOfxStatOK) ||
         (gPropSuite->propGetInt(props, kOfxPropIsInteractive, 0, &value) != kOfxStatOK) ) {
        return -1;
    }

    return value;
}

OfxStatus
describe(OfxImageEffectHandle effect)
{
    OfxPropertySetHandle props;
    OfxStatus stat = gEffectSuite->getPropertySet(effect, &props);

    if (stat != kOfxStatOK) {
        return stat;
    }
    gPropSuite->propSetString(props, kOfxPropLabel, 0, "InstanceEditTest");
    gPropSuite->propSetString(props, kOfxImageEffectPluginPropGrouping, 0, "Other/Test");
    gPropSuite->propSetString(props, kOfxImageEffectPropSupportedContexts, 0, kOfxImageEffectContextFilter);
    gPropSuite->propSetString(props, kOfxImageEffectPropSupportedPixelDepths, 0, kOfxBitDepthFloat);
    gPropSuite->propSetString(props, kOfxImageEffectPluginRenderThreadSafety, 0, kOfxImageEffectRenderFullySafe);

    return kOfxStatOK;
}

OfxStatus
describeInContext(OfxImageEffectHandle effect)
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
instanceEditPluginMain(const char* action,
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
        return describe(effect);
    }
    if (std::strcmp(action, kOfxImageEffectActionDescribeInContext) == 0) {
        return describeInContext(effect);
    }
    if (std::strcmp(action, kOfxActionCreateInstance) == 0) {
        natronTestIncrementCounter("InstanceEdit.created");

        return kOfxStatOK;
    }
    if (std::strcmp(action, kOfxActionBeginInstanceEdit) == 0) {
        bool inserted;
        {
            std::lock_guard<std::mutex> l(gMutex);
            inserted = gEditing.insert(effect).second;
        }
        natronTestIncrementCounter(inserted ? "InstanceEdit.begin" : "InstanceEdit.error.beginTwice");
        natronTestIncrementCounter( "InstanceEdit.interactiveInBegin", isInteractive(effect) );

        return kOfxStatOK;
    }
    if (std::strcmp(action, kOfxActionEndInstanceEdit) == 0) {
        bool erased;
        {
            std::lock_guard<std::mutex> l(gMutex);
            erased = gEditing.erase(effect) != 0;
        }
        natronTestIncrementCounter(erased ? "InstanceEdit.end" : "InstanceEdit.error.endWithoutBegin");
        natronTestIncrementCounter( "InstanceEdit.interactiveInEnd", isInteractive(effect) );

        return kOfxStatOK;
    }
    if (std::strcmp(action, kOfxActionDestroyInstance) == 0) {
        bool wasEditing;
        {
            std::lock_guard<std::mutex> l(gMutex);
            wasEditing = gEditing.erase(effect) != 0;
        }
        if (wasEditing) {
            natronTestIncrementCounter("InstanceEdit.error.destroyWhileEditing");
        }
        natronTestIncrementCounter("InstanceEdit.destroyed");

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
    kOfxImageEffectPluginApi, 1, "net.natron.test.InstanceEdit", 1, 0, setHost, instanceEditPluginMain
};
NatronTestPluginRegistrar gRegistrar(&gPlugin);
} // anonymous namespace
