/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2023 The Natron developers
 * (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
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

// Tests of the conversion of the viewer input to the viewer textures, and of the autocontrast range:
// - #1110: 8/16-bit images are read with their own type (32-bit textures and autocontrast);
// - #1111: the matte from a separate image is read at the right column (x1 + x);
// - #1118: 16-bit samples are not truncated to their low byte (Linear 16-bit LUT);
// - the alpha of 8/16-bit images is normalised to [0,1], and the A display (and its autocontrast) shows
//   alpha without the LUT of the input bit depth;
// - #1117: the project LUT choice "BT1886" gives eViewerColorSpaceBT1886;
// - #1337: the 8-bit texture applies the viewer gamma after the display curve, as the GL shader does.

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <algorithm> // min, max
#include <cmath> // pow
#include <cstring> // memset
#include <memory>
#include <vector>

#include <gtest/gtest.h>

#include "Engine/AppInstance.h"
#include "Engine/AppManager.h"
#include "Engine/Image.h"
#include "Engine/ImagePlaneDesc.h"
#include "Engine/KnobTypes.h"
#include "Engine/Lut.h"
#include "Engine/Project.h"
#include "Engine/ViewerInstancePrivate.h"

NATRON_NAMESPACE_USING

namespace {
const int kW = 128;
const int kH = 2;

ImagePtr
makeImage(ImageBitDepthEnum depth,
          const ImagePlaneDesc& comps = ImagePlaneDesc::getRGBAComponents(),
          int w = kW,
          int h = kH)
{
    ImagePtr img = std::make_shared<Image>( comps, RectD(0., 0., w, h), RectI(0, 0, w, h), 0, 1., depth,
                                            eImagePremultiplicationPremultiplied, eImageFieldingOrderNone );
    // the image buffer is not initialized
    Image::WriteAccess acc = img->getWriteRights();
    const std::size_t rowBytes = (std::size_t)w * img->getComponentsCount() * getSizeOfForBitDepth(depth);

    for (int y = 0; y < h; ++y) {
        std::memset(acc.pixelAt(0, y), 0, rowBytes);
    }

    return img;
}

// sets channel c of pixel (x, y) to the value v given as an 8-bit value (0..255), converted to the image depth
// the way a plug-in would write it: 8-bit v, 16-bit v * 257, float v / 255
void
setPixel8(const ImagePtr& img,
          int x,
          int y,
          int c,
          int v)
{
    Image::WriteAccess acc = img->getWriteRights();
    unsigned char* p = acc.pixelAt(x, y);

    ASSERT_TRUE(p != NULL);
    switch ( img->getBitDepth() ) {
    case eImageBitDepthByte:
        p[c] = (unsigned char)v;
        break;
    case eImageBitDepthShort:
        ( (unsigned short*)p )[c] = (unsigned short)(v * 257);
        break;
    case eImageBitDepthFloat:
        ( (float*)p )[c] = v / 255.f;
        break;
    default:
        FAIL();
    }
}

// an RGBA image: r = g = b = 32 + x (an 8-bit ramp), alpha = 255 - x
ImagePtr
makeRamp(ImageBitDepthEnum depth)
{
    ImagePtr img = makeImage(depth);

    for (int y = 0; y < kH; ++y) {
        for (int x = 0; x < kW; ++x) {
            for (int c = 0; c < 3; ++c) {
                setPixel8(img, x, y, c, 32 + x);
            }
            setPixel8(img, x, y, 3, 255 - x);
        }
    }

    return img;
}

RenderViewerArgs
makeArgs(const ImagePtr& img,
         DisplayChannelsEnum channels,
         int textureDepth,
         const Color::Lut* srcColorSpace,
         const ImagePtr& matte = ImagePtr(),
         int alphaChannelIndex = -1)
{
    const std::size_t tileRowElements = (textureDepth == eImageBitDepthFloat) ? kW * 4 : kW;

    return RenderViewerArgs(img, matte, channels, eImagePremultiplicationPremultiplied, textureDepth,
                            1. /*gain*/, 1. /*gamma*/, GammaLutConstPtr(), 0. /*offset*/,
                            srcColorSpace, NULL /*display colour space: none*/, alphaChannelIndex,
                            false /*renderOnlyRoI*/, tileRowElements);
}

// the tile covering columns [x1, kW) of the image (rectRounded = rect, so the tile buffer starts at x1)
UpdateViewerParams::CachedTile
makeTile(int x1 = 0)
{
    UpdateViewerParams::CachedTile tile;

    tile.rect = TextureRect(x1, 0, kW, kH, 1, 1.);
    tile.rectRounded = RectI(x1, 0, kW, kH);

    return tile;
}

// the 32-bit texture of the whole image (RGBA float, kW * 4 floats per row)
std::vector<float>
texture32(const RenderViewerArgs& args)
{
    std::vector<float> out(kW * kH * 4, -1.f);
    const RectI roi(0, 0, kW, kH);

    scaleToTexture32bits(roi, args, makeTile(), &out[0]);

    return out;
}

// the 8-bit texture of the whole image (BGRA U32, kW per row)
std::vector<U32>
texture8(const RenderViewerArgs& args)
{
    std::vector<U32> out(kW * kH, 0);
    const RectI roi(0, 0, kW, kH);

    scaleToTexture8bits(roi, args, NULL, makeTile(), &out[0]);

    return out;
}

inline int
alphaOf(U32 bgra)
{
    return (bgra >> 24) & 0xff;
}

inline int
redOf(U32 bgra)
{
    return (bgra >> 16) & 0xff;
}

const ImageBitDepthEnum kDepths[] = { eImageBitDepthByte, eImageBitDepthShort, eImageBitDepthFloat };
} // anonymous namespace

// #1118 and #1110: a 16-bit sample is read as unsigned short and normalised, not truncated to its low byte
// (which gave 0x34 / 65535 for 0x1234) or read as float (garbage).
TEST(ViewerTexture, Short0x1234IsNotTruncated)
{
    ImagePtr img = makeImage(eImageBitDepthShort);
    {
        Image::WriteAccess acc = img->getWriteRights();
        for (int y = 0; y < kH; ++y) {
            for (int x = 0; x < kW; ++x) {
                unsigned short* p = (unsigned short*)acc.pixelAt(x, y);
                p[0] = p[1] = p[2] = 0x1234;
                p[3] = 0xffff;
            }
        }
    }
    std::vector<float> tex = texture32( makeArgs(img, eDisplayChannelsRGB, eImageBitDepthFloat, NULL) );
    for (int i = 0; i < kW * kH; ++i) {
        ASSERT_NEAR(tex[i * 4], 0x1234 / 65535., 1e-6) << i;
        ASSERT_NEAR(tex[i * 4 + 1], 0x1234 / 65535., 1e-6) << i;
        ASSERT_NEAR(tex[i * 4 + 2], 0x1234 / 65535., 1e-6) << i;
        ASSERT_NEAR(tex[i * 4 + 3], 1., 1e-6) << i;
    }
    std::vector<U32> tex8 = texture8( makeArgs(img, eDisplayChannelsRGB, eImageBitDepthByte, NULL) );
    for (int i = 0; i < kW * kH; ++i) {
        ASSERT_EQ( redOf(tex8[i]), Color::floatToInt<256>(0x1234 / 65535.f) ) << i;
    }
}

// #1110: the same image in 8-bit, 16-bit and float gives the same 32-bit texture, with no LUT and with the
// sRGB LUT; the alpha of 8/16-bit images is in [0,1].
TEST(ViewerTexture, SameTextureForAllDepths)
{
    const Color::Lut* luts[] = { NULL, ViewerInstance::lutFromColorspace(eViewerColorSpaceSRGB) };

    for (int l = 0; l < 2; ++l) {
        std::vector<float> ref = texture32( makeArgs(makeRamp(eImageBitDepthFloat), eDisplayChannelsRGB, eImageBitDepthFloat, luts[l]) );
        for (int d = 0; d < 2; ++d) {
            std::vector<float> tex = texture32( makeArgs(makeRamp(kDepths[d]), eDisplayChannelsRGB, eImageBitDepthFloat, luts[l]) );
            // the "Fast" 8/16-bit LUTs are tables, the float conversion is exact
            const double tol = luts[l] ? 2e-3 : 1e-6;
            for (int i = 0; i < kW * kH * 4; ++i) {
                ASSERT_NEAR(tex[i], ref[i], tol) << "lut " << l << " depth " << kDepths[d] << " element " << i;
            }
        }
        // the float ramp itself
        for (int x = 0; x < kW; ++x) {
            const float v = (32 + x) / 255.f;
            EXPECT_NEAR(ref[x * 4], luts[l] ? luts[l]->fromColorSpaceFloatToLinearFloat(v) : v, 1e-6);
            EXPECT_NEAR(ref[x * 4 + 3], (255 - x) / 255., 1e-6);
        }
    }
}

// The alpha of 8/16-bit images is normalised in both texture paths: an alpha of 128 (8-bit) is about 0.5, not 1.
TEST(ViewerTexture, AlphaIsNormalised)
{
    for (int d = 0; d < 3; ++d) {
        ImagePtr img = makeRamp(kDepths[d]);
        std::vector<float> tex = texture32( makeArgs(img, eDisplayChannelsRGB, eImageBitDepthFloat, NULL) );
        std::vector<U32> tex8 = texture8( makeArgs(img, eDisplayChannelsRGB, eImageBitDepthByte, NULL) );
        for (int x = 0; x < kW; ++x) {
            EXPECT_NEAR(tex[x * 4 + 3], (255 - x) / 255., 1e-6) << "depth " << kDepths[d] << " x " << x;
            EXPECT_EQ(alphaOf(tex8[x]), 255 - x) << "depth " << kDepths[d] << " x " << x;
        }
    }
}

// #1111: with the matte from a separate image, the 32-bit path reads the matte at column x1 + x of a tile that
// starts at x1 = 64 (it used to read it at x), and uses the matte value, not the red channel, for 8/16-bit images.
TEST(ViewerTexture, MatteFromSeparateImageInTile)
{
    const int x1 = 64;

    for (int d = 0; d < 3; ++d) {
        ImagePtr color = makeImage(kDepths[d]); // black, alpha 0
        ImagePtr matte = makeImage(kDepths[d], ImagePlaneDesc::getAlphaComponents());
        for (int y = 0; y < kH; ++y) {
            for (int x = 0; x < kW; ++x) {
                setPixel8(matte, x, y, 0, 2 * x); // the matte ramp: 0, 2, ..., 254
            }
        }
        // the 32-bit texture of the tile [x1, kW)
        {
            RenderViewerArgs args = makeArgs(color, eDisplayChannelsMatte, eImageBitDepthFloat, NULL, matte, 0);
            std::vector<float> out(kW * kH * 4, -1.f);
            scaleToTexture32bits(RectI(0, 0, kW, kH), args, makeTile(x1), &out[0]);
            for (int x = x1; x < kW; ++x) {
                // r = 0 + matte * 0.5
                EXPECT_NEAR(out[(x - x1) * 4], 0.5 * (2 * x) / 255., 1e-6) << "depth " << kDepths[d] << " x " << x;
            }
        }
        // the 8-bit texture of the same tile
        {
            RenderViewerArgs args = makeArgs(color, eDisplayChannelsMatte, eImageBitDepthByte, NULL, matte, 0);
            std::vector<U32> out(kW * kH, 0);
            scaleToTexture8bits(RectI(0, 0, kW, kH), args, NULL, makeTile(x1), &out[0]);
            for (int x = x1; x < kW; ++x) {
                EXPECT_EQ(redOf(out[x - x1]), Color::floatToInt<256>( (2 * x) / 255.f ) / 2) << "depth " << kDepths[d] << " x " << x;
            }
        }
    }
}

// #1110: autocontrast gives the same range on the 8-bit, 16-bit and float versions of an image, with no LUT and
// with the sRGB LUT, and that range is the one of the texture values.
TEST(ViewerTexture, AutoContrastSameForAllDepths)
{
    const Color::Lut* luts[] = { NULL, ViewerInstance::lutFromColorspace(eViewerColorSpaceSRGB) };
    const DisplayChannelsEnum channels[] = { eDisplayChannelsRGB, eDisplayChannelsY, eDisplayChannelsR, eDisplayChannelsA };
    const RectI rect(0, 0, kW, kH);

    for (int l = 0; l < 2; ++l) {
        for (int c = 0; c < 4; ++c) {
            const int alphaIndex = (channels[c] == eDisplayChannelsA) ? 3 : -1;
            ImagePtr ref = makeRamp(eImageBitDepthFloat);
            MinMaxVal refRange = findAutoContrastVminVmax(ref, luts[l], channels[c], alphaIndex, rect);
            // the range of the texture values (r = g = b except for RGB, which has r = g = b too here)
            std::vector<float> tex = texture32( makeArgs(ref, channels[c], eImageBitDepthFloat, luts[l], ImagePtr(), alphaIndex) );
            double tmin = 1e30, tmax = -1e30;
            for (int i = 0; i < kW * kH; ++i) {
                tmin = std::min(tmin, (double)tex[i * 4]);
                tmax = std::max(tmax, (double)tex[i * 4]);
            }
            EXPECT_NEAR(refRange.min, tmin, 1e-6) << "lut " << l << " channels " << channels[c];
            EXPECT_NEAR(refRange.max, tmax, 1e-6) << "lut " << l << " channels " << channels[c];
            for (int d = 0; d < 2; ++d) {
                MinMaxVal range = findAutoContrastVminVmax(makeRamp(kDepths[d]), luts[l], channels[c], alphaIndex, rect);
                const double tol = luts[l] ? 2e-3 : 1e-6;
                EXPECT_NEAR(range.min, refRange.min, tol) << "lut " << l << " channels " << channels[c] << " depth " << kDepths[d];
                EXPECT_NEAR(range.max, refRange.max, tol) << "lut " << l << " channels " << channels[c] << " depth " << kDepths[d];
            }
        }
    }
}

// The A display shows alpha as is (alpha is linear): neither the texture nor autocontrast converts it with the LUT
// of the input bit depth (the default 8-bit LUT is sRGB), so an 8-bit alpha ramp 128..255 gives the range
// [128/255, 1] and the texture shows the same values.
TEST(ViewerTexture, AlphaDisplayHasNoLut)
{
    ImagePtr img = makeImage(eImageBitDepthByte);

    for (int y = 0; y < kH; ++y) {
        for (int x = 0; x < kW; ++x) {
            for (int c = 0; c < 3; ++c) {
                setPixel8(img, x, y, c, 0);
            }
            setPixel8(img, x, y, 3, 128 + x);
        }
    }
    const Color::Lut* srgb = ViewerInstance::lutFromColorspace(eViewerColorSpaceSRGB);
    MinMaxVal range = findAutoContrastVminVmax(img, srgb, eDisplayChannelsA, 3, RectI(0, 0, kW, kH));
    EXPECT_NEAR(range.min, 128 / 255., 1e-6);
    EXPECT_NEAR(range.max, 1., 1e-6);
    std::vector<float> tex = texture32( makeArgs(img, eDisplayChannelsA, eImageBitDepthFloat, srgb, ImagePtr(), 3) );
    std::vector<U32> tex8 = texture8( makeArgs(img, eDisplayChannelsA, eImageBitDepthByte, srgb, ImagePtr(), 3) );
    for (int x = 0; x < kW; ++x) {
        for (int c = 0; c < 3; ++c) {
            EXPECT_NEAR(tex[x * 4 + c], (128 + x) / 255., 1e-6) << "x " << x;
        }
        EXPECT_EQ(redOf(tex8[x]), 128 + x) << "x " << x;
    }

    // a one-channel alpha image: autocontrast reads channel alphaChannelIndex = 0, as the texture does
    ImagePtr alpha = makeImage(eImageBitDepthByte, ImagePlaneDesc::getAlphaComponents());
    for (int y = 0; y < kH; ++y) {
        for (int x = 0; x < kW; ++x) {
            setPixel8(alpha, x, y, 0, 64 + x);
        }
    }
    range = findAutoContrastVminVmax(alpha, srgb, eDisplayChannelsA, 0, RectI(0, 0, kW, kH));
    EXPECT_NEAR(range.min, 64 / 255., 1e-6);
    EXPECT_NEAR(range.max, (64 + kW - 1) / 255., 1e-6);
}

// #1337: the GL shader (Gui/Shaders.cpp) applies the gamma after the display curve; the 8-bit texture applied it
// before, to the linear values. sRGB is almost a power function, which commutes with the gamma, so the test uses
// dark values, where its linear toe makes the two orders differ by up to 20 levels.
TEST(ViewerTexture, GammaAfterDisplayCurve)
{
    ImagePtr img = makeImage(eImageBitDepthFloat);
    {
        Image::WriteAccess acc = img->getWriteRights();
        for (int y = 0; y < kH; ++y) {
            float* p = (float*)acc.pixelAt(0, y);
            for (int x = 0; x < kW; ++x) {
                p[x * 4] = p[x * 4 + 1] = p[x * 4 + 2] = x / 4000.f;
                p[x * 4 + 3] = 1.f;
            }
        }
    }
    const double gamma = 2.2;
    const Color::Lut* srgb = ViewerInstance::lutFromColorspace(eViewerColorSpaceSRGB);
    RenderViewerArgs args(img, ImagePtr(), eDisplayChannelsRGB, eImagePremultiplicationPremultiplied, eImageBitDepthByte,
                          1. /*gain*/, gamma, std::make_shared<GammaLut>(gamma), 0. /*offset*/,
                          NULL /*float input: linear*/, srgb, -1 /*alphaChannelIndex*/,
                          false /*renderOnlyRoI*/, kW);
    std::vector<U32> tex8 = texture8(args);
    for (int x = 0; x < kW; ++x) {
        const double expected = 255. * std::pow(srgb->toColorSpaceFloatFromLinearFloat(x / 4000.f), 1. / gamma);
        // error diffusion and the LUTs: at most 2 levels
        EXPECT_NEAR(redOf(tex8[x]), expected, 2.) << "x " << x;
    }
}

// #1117: the project LUT choices are Linear, sRGB, Rec.709 and BT1886 (Project Settings > LUT); "BT1886" used to
// give Linear. The same conversion is used by the viewer and by the bit-depth conversions of the renders.
TEST(ProjectLut, ChoiceIndexToColorSpace)
{
    AppInstancePtr app = appPTR->getTopLevelInstance();

    ASSERT_TRUE( bool(app) );
    const char* lutKnobs[] = { "defaultColorSpace8u", "defaultColorSpace16u", "defaultColorSpace32f" };
    const ImageBitDepthEnum depths[] = { eImageBitDepthByte, eImageBitDepthShort, eImageBitDepthFloat };
    const ViewerColorSpaceEnum expected[] = { eViewerColorSpaceLinear, eViewerColorSpaceSRGB, eViewerColorSpaceRec709, eViewerColorSpaceBT1886 };
    for (int i = 0; i < 3; ++i) {
        KnobChoicePtr lut = std::dynamic_pointer_cast<KnobChoice>( app->getProject()->getKnobByName(lutKnobs[i]) );
        ASSERT_TRUE( bool(lut) ) << lutKnobs[i];
        ASSERT_EQ(lut->getNumEntries(), 4) << lutKnobs[i];
        const int oldValue = lut->getValue();
        for (int choice = 0; choice < 4; ++choice) {
            lut->setValue(choice);
            EXPECT_EQ(app->getDefaultColorSpaceForBitDepth(depths[i]), expected[choice]) << lutKnobs[i] << " choice " << choice;
        }
        lut->setValue(oldValue);
    }
}
