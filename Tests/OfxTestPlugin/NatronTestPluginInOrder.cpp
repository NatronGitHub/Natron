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

// net.natron.test.InOrder (Tests/InOrderRender_Test.cpp, #248): a filter that, like WriteFFmpeg, needs its frames in
// order (kOfxImageEffectInstancePropSequentialRender = 1) and puts them in order itself while the host renders frames
// in parallel: the render of frame t waits in the render action until frame t-1 is done.
// - At an even frame t, it asks for the left half of the render window of Source (getRegionsOfInterest); in render,
//   it fetches the left half of Source at t-1 (which the host renders then, from getImage), keeps that image, and
//   waits until frame t-1 is done, for at most 5 s ("InOrder.timeouts" counts the waits that timed out; the bound
//   keeps a regression from hanging the test). "InOrder.contended" counts the waits that began before frame t-1 was
//   done: the case the test builds.
// - At an odd frame t, getRegionsOfInterest waits 300 ms, then asks for the whole render window of Source: the host
//   renders Source at t before the render action, into the cached image that frame t+1 may hold, which it must grow.
//   In render, it fetches that region.
// "InOrder.renders" counts the renders, and
// "InOrder.maxActive" is the largest number of renders that ran at the same time. The output is not written.

#include <chrono>
#include <climits>
#include <condition_variable>
#include <cstring>
#include <mutex>
#include <set>
#include <thread>

#include <ofxCore.h>
#include <ofxImageEffect.h>
#include <ofxProperty.h>

#include "NatronTestPluginRegistry.h"

namespace {
OfxHost* gHost = 0;
const OfxPropertySuiteV1* gPropSuite = 0;
const OfxImageEffectSuiteV1* gEffectSuite = 0;

const int kWaitMs = 5000;
const int kOddFrameDelayMs = 300;

std::mutex gActiveMutex;
int gActive = 0; // protected by gActiveMutex

std::mutex gDoneMutex;
std::condition_variable gDoneCond; // signalled when a frame is added to gDone
std::set<int> gDone; // the frames done, protected by gDoneMutex; cleared when an instance is created

void
setDone(int frame)
{
    {
        std::lock_guard<std::mutex> l(gDoneMutex);
        gDone.insert(frame);
    }
    gDoneCond.notify_all();
}

// waits until frame is done, for at most kWaitMs
void
waitForFrame(int frame)
{
    std::unique_lock<std::mutex> l(gDoneMutex);
    if ( gDone.count(frame) ) {
        return;
    }
    natronTestIncrementCounter("InOrder.contended");
    if ( !gDoneCond.wait_for( l, std::chrono::milliseconds(kWaitMs), [frame] { return gDone.count(frame) != 0; } ) ) {
        natronTestIncrementCounter("InOrder.timeouts");
    }
}

// counts the renders running at the same time, for its lifetime
class ActiveRender
{
public:
    ActiveRender()
    {
        std::lock_guard<std::mutex> l(gActiveMutex);
        ++gActive;
        if ( gActive > natronTestGetCounter("InOrder.maxActive") ) {
            natronTestSetCounter("InOrder.maxActive", gActive);
        }
    }

    ~ActiveRender()
    {
        std::lock_guard<std::mutex> l(gActiveMutex);
        --gActive;
    }

    ActiveRender(const ActiveRender&) = delete;
    ActiveRender& operator=(const ActiveRender&) = delete;
};

// fetches region of Source at time, and releases it at once (odd frames) or once waitFrame is done (even frames)
OfxStatus
fetchSource(OfxImageEffectHandle effect,
            double time,
            const OfxRectD& region,
            int waitFrame)
{
    OfxImageClipHandle source = 0;
    OfxStatus stat = gEffectSuite->clipGetHandle(effect, kOfxImageEffectSimpleSourceClipName, &source, 0);

    if (stat != kOfxStatOK) {
        return stat;
    }
    OfxPropertySetHandle image = 0;
    stat = gEffectSuite->clipGetImage(source, time, &region, &image);
    if (stat != kOfxStatOK) {
        return stat;
    }
    if (waitFrame != INT_MIN) {
        waitForFrame(waitFrame);
    }

    return gEffectSuite->clipReleaseImage(image);
}

OfxStatus
render(OfxImageEffectHandle effect,
       OfxPropertySetHandle inArgs)
{
    double time = 0.;
    int window[4] = {0, 0, 0, 0};

    gPropSuite->propGetDouble(inArgs, kOfxPropTime, 0, &time);
    gPropSuite->propGetIntN(inArgs, kOfxImageEffectPropRenderWindow, 4, window);
    natronTestIncrementCounter("InOrder.renders");
    const ActiveRender active;
    const int frame = static_cast<int>(time);
    OfxRectD region = { static_cast<double>(window[0]), static_cast<double>(window[1]), static_cast<double>(window[2]),
                        static_cast<double>(window[3]) };
    OfxStatus stat;
    if (frame % 2 == 0) {
        // the region of Source that getRegionsOfInterest asked for, at the previous frame
        region.x2 = (region.x1 + region.x2) / 2;
        stat = fetchSource(effect, time - 1, region, frame - 1);
    } else {
        stat = fetchSource(effect, time, region, INT_MIN);
    }
    setDone(frame);

    return stat;
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

OfxStatus
inOrderMain(const char* action,
            const void* handle,
            OfxPropertySetHandle inArgs,
            OfxPropertySetHandle outArgs)
{
    OfxImageEffectHandle effect = static_cast<OfxImageEffectHandle>( const_cast<void*>(handle) );

    if (std::strcmp(action, kOfxActionLoad) == 0) {
        if (!gHost) {
            return kOfxStatErrMissingHostFeature;
        }
        gPropSuite = static_cast<const OfxPropertySuiteV1*>( gHost->fetchSuite(gHost->host, kOfxPropertySuite, 1) );
        gEffectSuite = static_cast<const OfxImageEffectSuiteV1*>( gHost->fetchSuite(gHost->host, kOfxImageEffectSuite, 1) );

        return (gPropSuite && gEffectSuite) ? kOfxStatOK : kOfxStatErrMissingHostFeature;
    }
    if (std::strcmp(action, kOfxActionCreateInstance) == 0) {
        // a new test: no frame is done
        std::lock_guard<std::mutex> l(gDoneMutex);
        gDone.clear();

        return kOfxStatReplyDefault;
    }
    if (std::strcmp(action, kOfxActionDescribe) == 0) {
        OfxPropertySetHandle props;
        OfxStatus stat = gEffectSuite->getPropertySet(effect, &props);
        if (stat != kOfxStatOK) {
            return stat;
        }
        gPropSuite->propSetString(props, kOfxPropLabel, 0, "InOrderTest");
        gPropSuite->propSetString(props, kOfxImageEffectPluginPropGrouping, 0, "Other/Test");
        gPropSuite->propSetString(props, kOfxImageEffectPropSupportedContexts, 0, kOfxImageEffectContextFilter);
        gPropSuite->propSetString(props, kOfxImageEffectPropSupportedPixelDepths, 0, kOfxBitDepthFloat);
        gPropSuite->propSetString(props, kOfxImageEffectPluginRenderThreadSafety, 0, kOfxImageEffectRenderFullySafe);
        gPropSuite->propSetInt(props, kOfxImageEffectPluginPropHostFrameThreading, 0, 0);
        // one render action per frame
        gPropSuite->propSetInt(props, kOfxImageEffectPropSupportsTiles, 0, 0);
        // as WriteFFmpeg: it needs its frames in order
        gPropSuite->propSetInt(props, kOfxImageEffectInstancePropSequentialRender, 0, 1);

        return kOfxStatOK;
    }
    if (std::strcmp(action, kOfxImageEffectActionDescribeInContext) == 0) {
        OfxStatus stat = defineClip(effect, kOfxImageEffectSimpleSourceClipName);
        if (stat == kOfxStatOK) {
            stat = defineClip(effect, kOfxImageEffectOutputClipName);
        }

        return stat;
    }
    if (std::strcmp(action, kOfxImageEffectActionGetClipPreferences) == 0) {
        gPropSuite->propSetInt(outArgs, kOfxImageEffectFrameVarying, 0, 1);

        return kOfxStatOK;
    }
    if (std::strcmp(action, kOfxImageEffectActionGetRegionsOfInterest) == 0) {
        double time = 0.;
        double roi[4] = {0., 0., 0., 0.};
        gPropSuite->propGetDouble(inArgs, kOfxPropTime, 0, &time);
        gPropSuite->propGetDoubleN(inArgs, kOfxImageEffectPropRegionOfInterest, 4, roi);
        if (static_cast<int>(time) % 2 == 0) {
            roi[2] = (roi[0] + roi[2]) / 2;
        } else {
            // let frame t+1 fetch the left half of Source at t first
            std::this_thread::sleep_for( std::chrono::milliseconds(kOddFrameDelayMs) );
        }
        gPropSuite->propSetDoubleN(outArgs, "OfxImageClipPropRoI_" kOfxImageEffectSimpleSourceClipName, 4, roi);

        return kOfxStatOK;
    }
    if (std::strcmp(action, kOfxImageEffectActionRender) == 0) {
        return render(effect, inArgs);
    }

    return kOfxStatReplyDefault;
}

void
setHost(OfxHost* host)
{
    gHost = host;
}

OfxPlugin gPlugin = { kOfxImageEffectPluginApi, 1, "net.natron.test.InOrder", 1, 0, setHost, inOrderMain };
NatronTestPluginRegistrar gRegistrar(&gPlugin);
} // anonymous namespace
