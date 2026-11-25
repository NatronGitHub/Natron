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

// net.natron.test.ChoiceOrder (Tests/OfxChoiceOrder_Test.cpp): the UI order of the options of Choice params
// (kOfxParamPropChoiceOrder, OFX 1.5). Params:
// - ordered: options OptA, OptB, OptC, NewOpt with the order 0, 10, 30, 20 (the example of ofxParameter.rst,
//   with values that are not consecutive): displayed as OptA, OptB, NewOpt, OptC;
// - natural: options X, Y, Z, without an order;
// - partialOrder: options P, Q, R, with only two order values (the property must have one per option).
// Counters ("ChoiceOrder.*"): orderSet / orderFailed (the status of propSetInt for the order of "ordered":
// "To query whether a host supports this, a plugin should attempt to set the property and check the return
// status").

#include <cstring>

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

OfxStatus
loadSuites()
{
    if (!gHost) {
        return kOfxStatErrMissingHostFeature;
    }
    gPropSuite = (const OfxPropertySuiteV1*)gHost->fetchSuite(gHost->host, kOfxPropertySuite, 1);
    gEffectSuite = (const OfxImageEffectSuiteV1*)gHost->fetchSuite(gHost->host, kOfxImageEffectSuite, 1);
    gParamSuite = (const OfxParameterSuiteV1*)gHost->fetchSuite(gHost->host, kOfxParameterSuite, 1);

    return (gPropSuite && gEffectSuite && gParamSuite) ? kOfxStatOK : kOfxStatErrMissingHostFeature;
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

// a Choice param with the given options and, if nOrder > 0, the first nOrder values of order
void
defineChoice(OfxParamSetHandle paramSet,
             const char* name,
             const char* const* options,
             int nOptions,
             const int* order,
             int nOrder)
{
    OfxPropertySetHandle props = 0;

    if ( (gParamSuite->paramDefine(paramSet, kOfxParamTypeChoice, name, &props) != kOfxStatOK) || !props ) {
        return;
    }
    for (int i = 0; i < nOptions; ++i) {
        gPropSuite->propSetString(props, kOfxParamPropChoiceOption, i, options[i]);
    }
    bool orderSet = true;
    for (int i = 0; i < nOrder; ++i) {
        orderSet = (gPropSuite->propSetInt(props, kOfxParamPropChoiceOrder, i, order[i]) == kOfxStatOK) && orderSet;
    }
    if ( std::strcmp(name, "ordered") == 0 ) {
        natronTestIncrementCounter(orderSet ? "ChoiceOrder.orderSet" : "ChoiceOrder.orderFailed");
    }
}

OfxStatus
describeInContextChoiceOrder(OfxImageEffectHandle effect)
{
    OfxStatus stat = defineClips(effect);

    if (stat != kOfxStatOK) {
        return stat;
    }
    OfxParamSetHandle paramSet;
    stat = gEffectSuite->getParamSet(effect, &paramSet);
    if (stat != kOfxStatOK) {
        return stat;
    }
    const char* const ordered[] = { "OptA", "OptB", "OptC", "NewOpt" };
    const int orderedOrder[] = { 0, 10, 30, 20 };
    defineChoice(paramSet, "ordered", ordered, 4, orderedOrder, 4);
    const char* const natural[] = { "X", "Y", "Z" };
    defineChoice(paramSet, "natural", natural, 3, 0, 0);
    const char* const partial[] = { "P", "Q", "R" };
    const int partialOrder[] = { 5, 1 };
    defineChoice(paramSet, "partialOrder", partial, 3, partialOrder, 2);

    return kOfxStatOK;
}

OfxStatus
choiceOrderPluginMain(const char* action,
                      const void* handle,
                      OfxPropertySetHandle /*inArgs*/,
                      OfxPropertySetHandle /*outArgs*/)
{
    OfxImageEffectHandle effect = (OfxImageEffectHandle)handle;

    if (std::strcmp(action, kOfxActionLoad) == 0) {
        return loadSuites();
    }
    if (std::strcmp(action, kOfxActionDescribe) == 0) {
        return describe(effect, "ChoiceOrderTest");
    }
    if (std::strcmp(action, kOfxImageEffectActionDescribeInContext) == 0) {
        return describeInContextChoiceOrder(effect);
    }

    return kOfxStatReplyDefault;
}

void
setHost(OfxHost* host)
{
    gHost = host;
}

OfxPlugin gChoiceOrderPlugin = {
    kOfxImageEffectPluginApi, 1, "net.natron.test.ChoiceOrder", 1, 0, setHost, choiceOrderPluginMain
};
NatronTestPluginRegistrar gChoiceOrderRegistrar(&gChoiceOrderPlugin);
} // anonymous namespace
