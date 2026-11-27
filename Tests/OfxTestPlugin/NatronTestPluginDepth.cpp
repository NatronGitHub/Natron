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

// 8- and 16-bit images in the viewer at mipmap levels >= 1 (Tests/gui/test_viewer_8_16_bit_mipmap.py, #1549):
// - net.natron.test.Ramp: a float RGBA generator, 512x256, with R = G = B = x / 512 (canvas x of the
//   pixel centre) and A = 1;
// - net.natron.test.DepthByte and net.natron.test.DepthShort: filters that copy Source to Output and
//   support only 8-bit, respectively 16-bit images, so that the host converts the float input. They
//   don't support render scale (their region of definition fails at a scale other than 1), so that
//   the host renders them at full scale and downscales the result itself (Image::downscaleMipmap()).
// - net.natron.test.DepthWrite (Tests/OfxClipDepth_Test.cpp): a filter like DepthByte that supports 8-bit and
//   float images but not kOfxImageEffectPropSupportsMultipleClipDepths, and sets its output depth to 8 bits in
//   kOfxImageEffectActionGetClipPreferences, which only a plug-in that supports multiple clip depths may do
//   (#1385). Its render copies Source to Output and fails unless both are float images, the depth the host
//   must choose for a float input.

#include <cstddef>
#include <cstring>

#include <ofxCore.h>
#include <ofxImageEffect.h>
#include <ofxProperty.h>

#include "NatronTestPluginRegistry.h"

namespace {
OfxHost* gHost = 0;
const OfxPropertySuiteV1* gPropSuite = 0;
const OfxImageEffectSuiteV1* gEffectSuite = 0;

const int kRampWidth = 512;
const int kRampHeight = 256;

enum PluginEnum
{
    eRamp,
    eDepthByte,
    eDepthShort,
    eDepthWrite,
};

OfxStatus
defineClip(OfxImageEffectHandle effect,
           const char* name)
{
    OfxPropertySetHandle props;
    OfxStatus stat = gEffectSuite->clipDefine(effect, name, &props);

    if (stat == kOfxStatOK) {
        gPropSuite->propSetString(props, kOfxImageEffectPropSupportedComponents, 0, kOfxImageComponentRGBA);
    }

    return stat;
}

// An image fetched from a clip, released when it goes out of scope.
struct ClipImage
{
    OfxPropertySetHandle image = 0;
    void* data = 0;
    OfxRectI bounds = { 0, 0, 0, 0 };
    int rowBytes = 0;
    const char* depth = ""; // kOfxBitDepthByte...

    ClipImage(OfxImageEffectHandle effect,
              const char* clipName,
              OfxTime time)
    {
        OfxImageClipHandle clip = 0;
        if ( (gEffectSuite->clipGetHandle(effect, clipName, &clip, 0) != kOfxStatOK) ||
             (gEffectSuite->clipGetImage(clip, time, 0, &image) != kOfxStatOK) ) {
            image = 0;

            return;
        }
        if ( (gPropSuite->propGetPointer(image, kOfxImagePropData, 0, &data) != kOfxStatOK) ||
             (gPropSuite->propGetIntN(image, kOfxImagePropBounds, 4, &bounds.x1) != kOfxStatOK) ||
             (gPropSuite->propGetInt(image, kOfxImagePropRowBytes, 0, &rowBytes) != kOfxStatOK) ||
             (gPropSuite->propGetString(image, kOfxImageEffectPropPixelDepth, 0, &depth) != kOfxStatOK) ) {
            data = 0;
        }
    }

    ~ClipImage()
    {
        if (image) {
            gEffectSuite->clipReleaseImage(image);
        }
    }

    ClipImage(const ClipImage&) = delete;
    ClipImage& operator=(const ClipImage&) = delete;

    bool contains(int x,
                  int y) const
    {
        return bounds.x1 <= x && x < bounds.x2 && bounds.y1 <= y && y < bounds.y2;
    }

    // the first component of pixel (x, y), which must be in bounds
    template <typename PIX>
    PIX* pixelAt(int x,
                 int y) const
    {
        return (PIX*)( (char*)data + (std::ptrdiff_t)(y - bounds.y1) * rowBytes ) + 4 * (x - bounds.x1);
    }
};

// Copy Source to Output. Both must have the given depth: otherwise the host did not convert them
// (and the GUI test would compare a float image with itself), so the render fails.
template <typename PIX>
OfxStatus
copySource(OfxImageEffectHandle effect,
           OfxTime time,
           const OfxRectI& renderWindow,
           const char* depth)
{
    ClipImage dst(effect, kOfxImageEffectOutputClipName, time);
    ClipImage src(effect, kOfxImageEffectSimpleSourceClipName, time);

    if ( !dst.data || (std::strcmp(dst.depth, depth) != 0) || ( src.data && (std::strcmp(src.depth, depth) != 0) ) ) {
        return kOfxStatFailed;
    }
    for (int y = renderWindow.y1; y < renderWindow.y2; ++y) {
        for (int x = renderWindow.x1; x < renderWindow.x2; ++x) {
            if ( !dst.contains(x, y) ) {
                continue;
            }
            PIX* d = dst.pixelAt<PIX>(x, y);
            if ( src.data && src.contains(x, y) ) {
                std::memcpy( d, src.pixelAt<PIX>(x, y), 4 * sizeof(PIX) );
            } else {
                std::memset( d, 0, 4 * sizeof(PIX) );
            }
        }
    }

    return kOfxStatOK;
}

OfxStatus
renderRamp(OfxImageEffectHandle effect,
           OfxTime time,
           const OfxRectI& renderWindow,
           double scaleX)
{
    ClipImage dst(effect, kOfxImageEffectOutputClipName, time);

    if (!dst.data) {
        return kOfxStatFailed;
    }
    for (int y = renderWindow.y1; y < renderWindow.y2; ++y) {
        for (int x = renderWindow.x1; x < renderWindow.x2; ++x) {
            if ( !dst.contains(x, y) ) {
                continue;
            }
            float* d = dst.pixelAt<float>(x, y);
            d[0] = d[1] = d[2] = float( (x + 0.5) / scaleX / kRampWidth );
            d[3] = 1.f;
        }
    }

    return kOfxStatOK;
}

OfxStatus
depthPluginMain(PluginEnum plugin,
                const char* action,
                OfxImageEffectHandle effect,
                OfxPropertySetHandle inArgs,
                OfxPropertySetHandle outArgs)
{
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
        const char* const labels[] = { "RampTest", "DepthByteTest", "DepthShortTest", "DepthWriteTest" };
        const char* const depths[] = { kOfxBitDepthFloat, kOfxBitDepthByte, kOfxBitDepthShort, kOfxBitDepthByte };
        gPropSuite->propSetString(props, kOfxPropLabel, 0, labels[plugin]);
        gPropSuite->propSetString(props, kOfxImageEffectPluginPropGrouping, 0, "Other/Test");
        if (plugin == eRamp) {
            gPropSuite->propSetString(props, kOfxImageEffectPropSupportedContexts, 0, kOfxImageEffectContextGenerator);
        } else {
            gPropSuite->propSetString(props, kOfxImageEffectPropSupportedContexts, 0, kOfxImageEffectContextFilter);
        }
        gPropSuite->propSetString(props, kOfxImageEffectPropSupportedPixelDepths, 0, depths[plugin]);
        if (plugin == eDepthWrite) {
            gPropSuite->propSetString(props, kOfxImageEffectPropSupportedPixelDepths, 1, kOfxBitDepthFloat);
            gPropSuite->propSetInt(props, kOfxImageEffectPropSupportsMultipleClipDepths, 0, 0);
        }
        gPropSuite->propSetString(props, kOfxImageEffectPluginRenderThreadSafety, 0, kOfxImageEffectRenderFullySafe);

        return kOfxStatOK;
    }
    if (std::strcmp(action, kOfxImageEffectActionDescribeInContext) == 0) {
        OfxStatus stat = kOfxStatOK;
        if (plugin != eRamp) {
            stat = defineClip(effect, kOfxImageEffectSimpleSourceClipName);
        }
        if (stat == kOfxStatOK) {
            stat = defineClip(effect, kOfxImageEffectOutputClipName);
        }

        return stat;
    }
    if (std::strcmp(action, kOfxImageEffectActionGetRegionOfDefinition) == 0) {
        if (plugin != eRamp) {
            double scale[2] = { 1., 1. };
            gPropSuite->propGetDoubleN(inArgs, kOfxImageEffectPropRenderScale, 2, scale);

            // no render scale support; at scale 1, the default (the Source's region of definition)
            return (scale[0] == 1. && scale[1] == 1.) ? kOfxStatReplyDefault : kOfxStatFailed;
        }
        const double rod[4] = { 0., 0., double(kRampWidth), double(kRampHeight) };
        gPropSuite->propSetDoubleN(outArgs, kOfxImageEffectPropRegionOfDefinition, 4, rod);

        return kOfxStatOK;
    }
    if ( (plugin == eDepthWrite) && (std::strcmp(action, kOfxImageEffectActionGetClipPreferences) == 0) ) {
        // not allowed: the plug-in doesn't support multiple clip depths. The host keeps float.
        gPropSuite->propSetString(outArgs, "OfxImageClipPropDepth_" kOfxImageEffectOutputClipName, 0, kOfxBitDepthByte);

        return kOfxStatOK;
    }
    if (std::strcmp(action, kOfxImageEffectActionRender) == 0) {
        OfxTime time = 0.;
        OfxRectI renderWindow;
        double scale[2] = { 1., 1. };
        if ( (gPropSuite->propGetDouble(inArgs, kOfxPropTime, 0, &time) != kOfxStatOK) ||
             (gPropSuite->propGetIntN(inArgs, kOfxImageEffectPropRenderWindow, 4, &renderWindow.x1) != kOfxStatOK) ||
             (gPropSuite->propGetDoubleN(inArgs, kOfxImageEffectPropRenderScale, 2, scale) != kOfxStatOK) ) {
            return kOfxStatFailed;
        }
        if ( (plugin != eRamp) && ( (scale[0] != 1.) || (scale[1] != 1.) ) ) {
            return kOfxStatFailed; // the host must not render them at a scale other than 1 (see above)
        }
        switch (plugin) {
        case eRamp:

            return renderRamp(effect, time, renderWindow, scale[0]);
        case eDepthByte:

            return copySource<unsigned char>(effect, time, renderWindow, kOfxBitDepthByte);
        case eDepthShort:

            return copySource<unsigned short>(effect, time, renderWindow, kOfxBitDepthShort);
        case eDepthWrite:

            return copySource<float>(effect, time, renderWindow, kOfxBitDepthFloat);
        }
    }

    return kOfxStatReplyDefault;
}

OfxStatus
rampMain(const char* action,
         const void* handle,
         OfxPropertySetHandle inArgs,
         OfxPropertySetHandle outArgs)
{
    return depthPluginMain(eRamp, action, (OfxImageEffectHandle)handle, inArgs, outArgs);
}

OfxStatus
depthByteMain(const char* action,
              const void* handle,
              OfxPropertySetHandle inArgs,
              OfxPropertySetHandle outArgs)
{
    return depthPluginMain(eDepthByte, action, (OfxImageEffectHandle)handle, inArgs, outArgs);
}

OfxStatus
depthShortMain(const char* action,
               const void* handle,
               OfxPropertySetHandle inArgs,
               OfxPropertySetHandle outArgs)
{
    return depthPluginMain(eDepthShort, action, (OfxImageEffectHandle)handle, inArgs, outArgs);
}

OfxStatus
depthWriteMain(const char* action,
               const void* handle,
               OfxPropertySetHandle inArgs,
               OfxPropertySetHandle outArgs)
{
    return depthPluginMain(eDepthWrite, action, (OfxImageEffectHandle)handle, inArgs, outArgs);
}

void
setHost(OfxHost* host)
{
    gHost = host;
}

OfxPlugin gPlugins[] = {
    { kOfxImageEffectPluginApi, 1, "net.natron.test.Ramp", 1, 0, setHost, rampMain },
    { kOfxImageEffectPluginApi, 1, "net.natron.test.DepthByte", 1, 0, setHost, depthByteMain },
    { kOfxImageEffectPluginApi, 1, "net.natron.test.DepthShort", 1, 0, setHost, depthShortMain },
    { kOfxImageEffectPluginApi, 1, "net.natron.test.DepthWrite", 1, 0, setHost, depthWriteMain },
};
NatronTestPluginRegistrar gRegistrars[] = {
    NatronTestPluginRegistrar(&gPlugins[0]), NatronTestPluginRegistrar(&gPlugins[1]), NatronTestPluginRegistrar(&gPlugins[2]),
    NatronTestPluginRegistrar(&gPlugins[3]),
};
} // anonymous namespace
