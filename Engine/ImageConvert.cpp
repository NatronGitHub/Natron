/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2016 INRIA and Alexandre Gauthier-Foichat
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

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "ImagePrivate.h"

#include <algorithm> // min, max
#include <cassert>
#include <stdexcept>

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_OFF
#include <boost/math/special_functions/fpclassify.hpp>
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_ON
#endif

#include <QtCore/QDebug>

#include "Engine/AppManager.h"
#include "Engine/Lut.h"

NATRON_NAMESPACE_ENTER;

///explicit template instantiations

template <>
float
Image::convertPixelDepth(unsigned char pix)
{
    return Color::intToFloat<256>(pix);
}

template <>
unsigned short
Image::convertPixelDepth(unsigned char pix)
{
    // 0x01 -> 0x0101, 0x02 -> 0x0202, ..., 0xff -> 0xffff
    return (unsigned short)( (pix << 8) + pix );
}

template <>
unsigned char
Image::convertPixelDepth(unsigned char pix)
{
    return pix;
}

template <>
unsigned char
Image::convertPixelDepth(unsigned short pix)
{
    // the following is from ImageMagick's quantum.h
    return (unsigned char)( ( (pix + 128UL) - ( (pix + 128UL) >> 8 ) ) >> 8 );
}

template <>
float
Image::convertPixelDepth(unsigned short pix)
{
    return Color::intToFloat<65536>(pix);
}

template <>
unsigned short
Image::convertPixelDepth(unsigned short pix)
{
    return pix;
}

template <>
unsigned char
Image::convertPixelDepth(float pix)
{
    return (unsigned char)Color::floatToInt<256>(pix);
}

template <>
unsigned short
Image::convertPixelDepth(float pix)
{
    return (unsigned short)Color::floatToInt<65536>(pix);
}

template <>
float
Image::convertPixelDepth(float pix)
{
    return pix;
}

static const Color::Lut*
lutFromColorspace(ViewerColorSpaceEnum cs)
{
    const Color::Lut* lut;

    switch (cs) {
    case eViewerColorSpaceSRGB:
        lut = Color::LutManager::sRGBLut();
        break;
    case eViewerColorSpaceRec709:
        lut = Color::LutManager::Rec709Lut();
        break;
    case eViewerColorSpaceLinear:
    default:
        lut = 0;
        break;
    }
    if (lut) {
        lut->validate();
    }

    return lut;
}

///Fast version when components are the same
template <typename SRCPIX, typename DSTPIX, int srcMaxValue, int dstMaxValue>
static void
convertToFormatInternal_sameComps(const RectI & renderWindow,
                                  ViewerColorSpaceEnum srcColorSpace,
                                  ViewerColorSpaceEnum dstColorSpace,
                                  const void* srcBuf,
                                  int nComp,
                                  const RectI& srcBounds,
                                  void* dstBuf,
                                  const RectI& dstBounds)
{

    const Color::Lut* const srcLut_ = lutFromColorspace(srcColorSpace);
    const Color::Lut* const dstLut_ = lutFromColorspace(dstColorSpace);


    // no colorspace conversion applied when luts are the same
    const Color::Lut* const srcLut = (srcLut_ == dstLut_) ? 0 : srcLut_;
    const Color::Lut* const dstLut = (srcLut_ == dstLut_) ? 0 : dstLut_;

    int dstDataSizeOf = sizeof(DSTPIX);
    int srcDataSizeOf = sizeof(SRCPIX);


    for (int y = 0; y < renderWindow.height(); ++y) {


        if (srcMaxValue == dstMaxValue && !srcLut && !dstLut) {
            // Use memcpy when possible

            const SRCPIX* srcPixels = (const SRCPIX*)Image::pixelAtStatic(renderWindow.x1, renderWindow.y1 + y, srcBounds, nComp, srcDataSizeOf, (unsigned char*)srcBuf);
            DSTPIX* dstPixels = (DSTPIX*)Image::pixelAtStatic(renderWindow.x1, renderWindow.y1 + y, dstBounds, nComp, dstDataSizeOf, (unsigned char*)dstBuf);

            memcpy(dstPixels, srcPixels, renderWindow.width() * nComp * srcDataSizeOf);
        } else {
            // Start of the line for error diffusion
            // coverity[dont_call]
            int start = rand() % renderWindow.width();
            const SRCPIX* srcPixels = (const SRCPIX*)Image::pixelAtStatic(renderWindow.x1 + start, renderWindow.y1 + y, srcBounds, nComp, srcDataSizeOf, (unsigned char*)srcBuf);
            DSTPIX* dstPixels = (DSTPIX*)Image::pixelAtStatic(renderWindow.x1 + start, renderWindow.y1 + y, dstBounds, nComp, dstDataSizeOf, (unsigned char*)dstBuf);

            const SRCPIX* srcStart = srcPixels;
            DSTPIX* dstStart = dstPixels;

            for (int backward = 0; backward < 2; ++backward) {
                int x = backward ? start - 1 : start;
                int end = backward ? -1 : renderWindow.width();
                unsigned error[3] = {
                    0x80, 0x80, 0x80
                };

                while ( x != end && x >= 0 && x < renderWindow.width() ) {
                    for (int k = 0; k < nComp; ++k) {
#                 ifdef DEBUG
                        assert(srcPixels[k] == srcPixels[k]); // check for NaN
#                 endif
                        DSTPIX pix;
                        if ( (k == 3) || (!srcLut && !dstLut) ) {
                            pix = Image::convertPixelDepth<SRCPIX, DSTPIX>(srcPixels[k]);
                        } else {
                            float pixFloat;

                            if (srcLut) {
                                if (srcMaxValue == 255) {
                                    pixFloat = srcLut->fromColorSpaceUint8ToLinearFloatFast(srcPixels[k]);
                                } else if (srcMaxValue == 65535) {
                                    pixFloat = srcLut->fromColorSpaceUint16ToLinearFloatFast(srcPixels[k]);
                                } else {
                                    pixFloat = srcLut->fromColorSpaceFloatToLinearFloat(srcPixels[k]);
                                }
                            } else {
                                pixFloat = Image::convertPixelDepth<SRCPIX, float>(srcPixels[k]);
                            }

                            if (dstMaxValue == 255) {
                                ///small increase in perf we use Luts. This should be anyway the most used case.
                                error[k] = (error[k] & 0xff) + ( dstLut ? dstLut->toColorSpaceUint8xxFromLinearFloatFast(pixFloat) :
                                                                Color::floatToInt<0xff01>(pixFloat) );
                                pix = error[k] >> 8;
                            } else if (dstMaxValue == 65535) {
                                pix = dstLut ? dstLut->toColorSpaceUint16FromLinearFloatFast(pixFloat) :
                                Image::convertPixelDepth<float, DSTPIX>(pixFloat);
                            } else {
                                if (dstLut) {
                                    pixFloat = dstLut->toColorSpaceFloatFromLinearFloat(pixFloat);
                                }
                                pix = Image::convertPixelDepth<float, DSTPIX>(pixFloat);
                            }
                        }
                        dstPixels[k] =  pix;
#                 ifdef DEBUG
                        assert(dstPixels[k] == dstPixels[k]); // check for NaN
#                 endif
                    }

                    if (backward) {
                        --x;
                        srcPixels -= nComp;
                        dstPixels -= nComp;
                    } else {
                        ++x;
                        srcPixels += nComp;
                        dstPixels += nComp;
                    }
                }
                srcPixels = srcStart - nComp;
                dstPixels = dstStart - nComp;
            } // backward
        } // !useMemcpy
    } // for all lines
} // convertToFormatInternal_sameComps

template <typename SRCPIX, typename DSTPIX, int srcMaxValue, int dstMaxValue, int srcNComps, int dstNComps, bool requiresUnpremult, bool useColorspaces>
void
static convertToFormatInternalForColorSpace(const RectI & renderWindow,
                                            ViewerColorSpaceEnum srcColorSpace,
                                            ViewerColorSpaceEnum dstColorSpace,
                                            int conversionChannel,
                                            ImagePrivate::AlphaChannelHandlingEnum alphaHandling,
                                            const void* srcBuf,
                                            const RectI& srcBounds,
                                            void* dstBuf,
                                            const RectI& dstBounds)
{
    // Other cases are optimizes in convertFromMono and convertToMono
    assert(dstNComps > 1 && srcNComps > 1);

    int dstDataSizeOf = sizeof(DSTPIX);
    int srcDataSizeOf = sizeof(SRCPIX);


    const Color::Lut* const srcLut = useColorspaces ? lutFromColorspace( (ViewerColorSpaceEnum)srcColorSpace ) : 0;
    const Color::Lut* const dstLut = useColorspaces ? lutFromColorspace( (ViewerColorSpaceEnum)dstColorSpace ) : 0;

    for (int y = renderWindow.y1; y < renderWindow.y2; ++y) {
        // Start of the line for error diffusion
        // coverity[dont_call]

        int start = rand() % renderWindow.width();
        const SRCPIX* srcPixels = (const SRCPIX*)Image::pixelAtStatic(renderWindow.x1 + start, y, srcBounds, srcNComps, srcDataSizeOf, (unsigned char*)srcBuf);
        DSTPIX* dstPixels = (DSTPIX*)Image::pixelAtStatic(renderWindow.x1 + start, y, dstBounds, dstNComps, dstDataSizeOf, (unsigned char*)dstBuf);
        const SRCPIX* srcStart = srcPixels;
        DSTPIX* dstStart = dstPixels;

        for (int backward = 0; backward < 2; ++backward) {
            ///We do twice the loop, once from starting point to end and once from starting point - 1 to real start
            int x = backward ? start - 1 : start;

            //End is pointing to the first pixel outside the line a la stl
            int end = backward ? -1 : renderWindow.width();
            unsigned error[3] = {
                0x80, 0x80, 0x80
            };

            while ( x != end && x >= 0 && x < renderWindow.width() ) {

                // We've XY, RGB or RGBA input and outputs
                assert(srcNComps != dstNComps);

                const bool unpremultChannel = ( //srcNComps == 4 && // test already done in convertToFormatInternalForDepth
                                               //dstNComps == 3 && // test already done in convertToFormatInternalForDepth
                                               requiresUnpremult);

                ///This is only set if unpremultChannel is true
                float alphaForUnPremult;
                if (unpremultChannel) {
                    alphaForUnPremult = Image::convertPixelDepth<SRCPIX, float>(srcPixels[srcNComps - 1]);
                } else {
                    alphaForUnPremult = 1.;
                }

                for (int k = 0; k < 3 && k < dstNComps; ++k) {
                    SRCPIX sourcePixel = srcPixels[k];
                    DSTPIX pix;
                    if ( !useColorspaces || (!srcLut && !dstLut) ) {
                        if (dstMaxValue == 255) {
                            float pixFloat = Image::convertPixelDepth<SRCPIX, float>(sourcePixel);
                            error[k] = (error[k] & 0xff) + Color::floatToInt<0xff01>(pixFloat);
                            pix = error[k] >> 8;
                        } else {
                            pix = Image::convertPixelDepth<SRCPIX, DSTPIX>(sourcePixel);
                        }
                    } else {
                        ///For RGB channels
                        float pixFloat;

                        ///Unpremult before doing colorspace conversion from linear to X
                        if (unpremultChannel) {
                            pixFloat = Image::convertPixelDepth<SRCPIX, float>(sourcePixel);
                            pixFloat = alphaForUnPremult == 0.f ? 0. : pixFloat / alphaForUnPremult;
                            if (srcLut) {
                                pixFloat = srcLut->fromColorSpaceFloatToLinearFloat(pixFloat);
                            }
                        } else if (srcLut) {
                            if (srcMaxValue == 255) {
                                pixFloat = srcLut->fromColorSpaceUint8ToLinearFloatFast(sourcePixel);
                            } else if (srcMaxValue == 65535) {
                                pixFloat = srcLut->fromColorSpaceUint16ToLinearFloatFast(sourcePixel);
                            } else {
                                pixFloat = srcLut->fromColorSpaceFloatToLinearFloat(sourcePixel);
                            }
                        } else {
                            pixFloat = Image::convertPixelDepth<SRCPIX, float>(sourcePixel);
                        }

                        ///Apply dst color-space
                        if (dstMaxValue == 255) {
                            assert(k < 3);
                            error[k] = (error[k] & 0xff) + ( dstLut ? dstLut->toColorSpaceUint8xxFromLinearFloatFast(pixFloat) :
                                                            Color::floatToInt<0xff01>(pixFloat) );
                            pix = error[k] >> 8;
                        } else if (dstMaxValue == 65535) {
                            pix = dstLut ? dstLut->toColorSpaceUint16FromLinearFloatFast(pixFloat) :
                            Image::convertPixelDepth<float, DSTPIX>(pixFloat);
                        } else {
                            if (dstLut) {
                                pixFloat = dstLut->toColorSpaceFloatFromLinearFloat(pixFloat);
                            }
                            pix = Image::convertPixelDepth<float, DSTPIX>(pixFloat);
                        }
                    } // if (!useColorspaces || (!srcLut && !dstLut)) {
                    dstPixels[k] =  pix;
#                 ifdef DEBUG
                    assert(dstPixels[k] == dstPixels[k]); // check for NaN
#                 endif
                } // for (int k = 0; k < k < 3 && k < dstNComps; ++k) {

                if (dstNComps == 4 && srcNComps < 4) {
                    // we reach here only if converting RGB-->RGBA or XY--->RGBA
                    DSTPIX pix;
                    switch (alphaHandling) {
                        case ImagePrivate::eAlphaChannelHandlingCreateFill0:
                            pix = 0;
                            break;
                        case ImagePrivate::eAlphaChannelHandlingCreateFill1:
                            pix = (DSTPIX)dstMaxValue;
                            break;
                        case ImagePrivate::eAlphaChannelHandlingFillFromChannel:
                            assert(conversionChannel >= 0 && conversionChannel < srcNComps);
                            Image::convertPixelDepth<SRCPIX, DSTPIX>(srcPixels[conversionChannel]);
                            break;
                    }

                    dstPixels[3] =  pix;
                }
                if (backward) {
                    --x;
                    srcPixels -= srcNComps;
                    dstPixels -= dstNComps;
                } else {
                    ++x;
                    srcPixels += srcNComps;
                    dstPixels += dstNComps;
                }
            } // while ( x != end && x >= 0 && x < renderWindow.width() ) {
            srcPixels = srcStart - srcNComps;
            dstPixels = dstStart - dstNComps;
        } // for (int backward = 0; backward < 2; ++backward) {
    }  // for (int y = 0; y < renderWindow.height(); ++y) {

} // convertToFormatInternalForColorSpace

template <typename SRCPIX, typename DSTPIX, int srcMaxValue, int dstMaxValue, int srcNComps, Image::AlphaChannelHandlingEnum alphaHandling>
void
static convertToMonoImage(const RectI & renderWindow,
                          int conversionChannel,
                          const void* srcBuf,
                          const RectI& srcBounds,
                          void* dstBuf,
                          const RectI& dstBounds)
{
    int dstDataSizeOf = sizeof(DSTPIX);
    int srcDataSizeOf = sizeof(SRCPIX);

    DSTPIX pix;

    for (int y = renderWindow.y1; y < renderWindow.y2; ++y) {
        // Start of the line for error diffusion
        // coverity[dont_call]

        const SRCPIX* srcPixels = (const SRCPIX*)Image::pixelAtStatic(renderWindow.x1, y, srcBounds, srcNComps, srcDataSizeOf, (unsigned char*)srcBuf);
        DSTPIX* dstPixels = (DSTPIX*)Image::pixelAtStatic(renderWindow.x1, y, dstBounds, 1, dstDataSizeOf, (unsigned char*)dstBuf);

        for (int x = renderWindow.x1; x < renderWindow.x2; ++x, srcPixels += srcNComps, ++dstPixels) {
            switch (alphaHandling) {
                case Image::eAlphaChannelHandlingCreateFill0:
                    pix = 0;
                    break;
                case Image::eAlphaChannelHandlingCreateFill1:
                    pix = (DSTPIX)dstMaxValue;
                    break;
                case Image::eAlphaChannelHandlingFillFromChannel:
                    assert(conversionChannel >= 0 && conversionChannel < srcNComps);
                    Image::convertPixelDepth<SRCPIX, DSTPIX>(srcPixels[conversionChannel]);

                    break;
                    
            }
            *dstPixels = pix;
        }
    }
} // convertToMonoImage

template <typename SRCPIX, typename DSTPIX, int srcMaxValue, int dstMaxValue, int dstNComps, Image::MonoToPackedConversionEnum monoConversion>
void
static convertFromMonoImage(const RectI & renderWindow,
                          int conversionChannel,
                          const void* srcBuf,
                          const RectI& srcBounds,
                          void* dstBuf,
                          const RectI& dstBounds)
{
    int dstDataSizeOf = sizeof(DSTPIX);
    int srcDataSizeOf = sizeof(SRCPIX);

    DSTPIX pix;

    for (int y = renderWindow.y1; y < renderWindow.y2; ++y) {
        // Start of the line for error diffusion
        // coverity[dont_call]

        const SRCPIX* srcPixels = (const SRCPIX*)Image::pixelAtStatic(renderWindow.x1, y, srcBounds, 1, srcDataSizeOf, (unsigned char*)srcBuf);
        DSTPIX* dstPixels = (DSTPIX*)Image::pixelAtStatic(renderWindow.x1, y, dstBounds, dstNComps, dstDataSizeOf, (unsigned char*)dstBuf);

        for (int x = renderWindow.x1; x < renderWindow.x2; ++x, ++srcPixels, dstPixels += dstNComps) {

            pix = Image::convertPixelDepth<SRCPIX, DSTPIX>(*srcPixels);

            switch (monoConversion) {
                case Image::eMonoToPackedConversionCopyToAll: {
                    for (int c = 0; c < dstNComps; ++c) {
                        dstPixels[c] = pix;
                    }
                }   break;
                case Image::eMonoToPackedConversionCopyToChannelAndFillOthers: {
                    assert(conversionChannel >= 0 && conversionChannel < dstNComps);
                    for (int c = 0; c < dstNComps; ++c) {
                        if (c == conversionChannel) {
                            dstPixels[c] = pix;
                        } else {
                            // Fill
                            dstPixels[c] = 0;
                        }
                    }

                }   break;
                case Image::eMonoToPackedConversionCopyToChannelAndLeaveOthers:
                    dstPixels[conversionChannel] = pix;
                    break;

            }
            *dstPixels = pix;
        }
    }
} // convertFromMonoImage


template <typename SRCPIX, typename DSTPIX, int srcMaxValue, int dstMaxValue, int srcNComps, int dstNComps,
          bool requiresUnpremult>
static void
convertToFormatInternalForUnpremult(const RectI & renderWindow,
                                    ViewerColorSpaceEnum srcColorSpace,
                                    ViewerColorSpaceEnum dstColorSpace,
                                    int conversionChannel,
                                    Image::AlphaChannelHandlingEnum alphaHandling,
                                    const void* srcBuf,
                                    const RectI& srcBounds,
                                    void* dstBuf,
                                    const RectI& dstBounds)
{
    if ( (srcColorSpace == eViewerColorSpaceLinear) && (dstColorSpace == eViewerColorSpaceLinear) ) {
        convertToFormatInternalForColorSpace<SRCPIX, DSTPIX, srcMaxValue, dstMaxValue, srcNComps, dstNComps, requiresUnpremult, false>(renderWindow, srcColorSpace, dstColorSpace, conversionChannel, alphaHandling, srcBuf, srcBounds, dstBuf, dstBounds);
    } else {
        convertToFormatInternalForColorSpace<SRCPIX, DSTPIX, srcMaxValue, dstMaxValue, srcNComps, dstNComps, requiresUnpremult, true>(renderWindow, srcColorSpace, dstColorSpace, conversionChannel, alphaHandling, srcBuf, srcBounds, dstBuf, dstBounds);
    }
}

template <typename SRCPIX, typename DSTPIX, int srcMaxValue, int dstMaxValue, int srcNComps, int dstNComps>
static void
convertToFormatInternal(const RectI & renderWindow,
                        ViewerColorSpaceEnum srcColorSpace,
                        ViewerColorSpaceEnum dstColorSpace,
                        bool requiresUnpremult,
                        int conversionChannel,
                        Image::AlphaChannelHandlingEnum alphaHandling,
                        Image::MonoToPackedConversionEnum monoConversion,
                        const void* srcBuf,
                        const RectI& srcBounds,
                        void* dstBuf,
                        const RectI& dstBounds)
{
    if (dstNComps == 1) {
        // When converting to alpha, do not colorspace conversion
        // optimize the conversion
        switch (alphaHandling) {
            case Image::eAlphaChannelHandlingFillFromChannel:
                convertToMonoImage<SRCPIX, DSTPIX, srcMaxValue, dstMaxValue, srcNComps, ImagePrivate::eAlphaChannelHandlingFillFromChannel>(renderWindow, conversionChannel, srcBuf, srcBounds, dstBuf, dstBounds);
                break;
            case Image::eAlphaChannelHandlingCreateFill1:
                convertToMonoImage<SRCPIX, DSTPIX, srcMaxValue, dstMaxValue, srcNComps, ImagePrivate::eAlphaChannelHandlingCreateFill1>(renderWindow, conversionChannel, srcBuf, srcBounds, dstBuf, dstBounds);
                break;
            case Image::eAlphaChannelHandlingCreateFill0:
                convertToMonoImage<SRCPIX, DSTPIX, srcMaxValue, dstMaxValue, srcNComps, ImagePrivate::eAlphaChannelHandlingCreateFill0>(renderWindow, conversionChannel, srcBuf, srcBounds, dstBuf, dstBounds);
                break;
        }
    } else if (srcNComps == 1) {
        // Mono image to something else, optimize
        switch (monoConversion) {
            case Image::eMonoToPackedConversionCopyToChannelAndLeaveOthers:
                convertFromMonoImage<SRCPIX, DSTPIX, srcMaxValue, dstMaxValue, srcNComps, ImagePrivate::eMonoToPackedConversionCopyToChannelAndLeaveOthers>(renderWindow, conversionChannel, srcBuf, srcBounds, dstBuf, dstBounds);
                break;
            case Image::eMonoToPackedConversionCopyToChannelAndFillOthers:
                convertFromMonoImage<SRCPIX, DSTPIX, srcMaxValue, dstMaxValue, srcNComps, ImagePrivate::eMonoToPackedConversionCopyToChannelAndFillOthers>(renderWindow, conversionChannel, srcBuf, srcBounds, dstBuf, dstBounds);
                break;
            case Image::eMonoToPackedConversionCopyToAll:
                convertFromMonoImage<SRCPIX, DSTPIX, srcMaxValue, dstMaxValue, srcNComps, ImagePrivate::eMonoToPackedConversionCopyToAll>(renderWindow, conversionChannel, srcBuf, srcBounds, dstBuf, dstBounds);
                break;
        }
    } else {
        // General case
        if (requiresUnpremult) {
            convertToFormatInternalForUnpremult<SRCPIX, DSTPIX, srcMaxValue, dstMaxValue, srcNComps, dstNComps, true>(renderWindow, srcColorSpace, dstColorSpace, conversionChannel, alphaHandling, srcBuf, srcBounds, dstBuf, dstBounds);
        } else {
            convertToFormatInternalForUnpremult<SRCPIX, DSTPIX, srcMaxValue, dstMaxValue, srcNComps, dstNComps, false>(renderWindow, srcColorSpace, dstColorSpace, conversionChannel, alphaHandling, srcBuf, srcBounds, dstBuf, dstBounds);
        }
    }
}

template <typename SRCPIX, typename DSTPIX, int srcMaxValue, int dstMaxValue>
static void
convertToFormatInternalForDepth(const RectI & renderWindow,
                                ViewerColorSpaceEnum srcColorSpace,
                                ViewerColorSpaceEnum dstColorSpace,
                                bool requiresUnpremult,
                                int conversionChannel,
                                Image::AlphaChannelHandlingEnum alphaHandling,
                                Image::MonoToPackedConversionEnum monoConversion,
                                const void* srcBuf,
                                int srcNComps,
                                const RectI& srcBounds,
                                void* dstBuf,
                                int dstNComps,
                                const RectI& dstBounds)
{

    // see convertToFormatInternalForColorSpace : it is only used in one case!
    assert(!requiresUnpremult || (srcNComps == 4 && dstNComps == 3));

    switch (srcNComps) {
        case 1:
            switch (dstNComps) {
                case 2:
                    convertToFormatInternal<SRCPIX, DSTPIX, srcMaxValue, dstMaxValue, 1, 2>(renderWindow, srcColorSpace, dstColorSpace, requiresUnpremult, conversionChannel, alphaHandling, monoConversion, srcBuf, srcBounds, dstBuf, dstBounds);
                    break;
                case 3:
                    convertToFormatInternal<SRCPIX, DSTPIX, srcMaxValue, dstMaxValue, 1, 3>(renderWindow, srcColorSpace, dstColorSpace, requiresUnpremult, conversionChannel, alphaHandling, monoConversion, srcBuf, srcBounds, dstBuf, dstBounds);
                    break;
                case 4:
                    convertToFormatInternal<SRCPIX, DSTPIX, srcMaxValue, dstMaxValue, 1, 4>(renderWindow, srcColorSpace, dstColorSpace, requiresUnpremult, conversionChannel, alphaHandling, monoConversion, srcBuf, srcBounds, dstBuf, dstBounds);
                    break;
                default:
                    assert(false);
                    break;
            }
            break;
        case 2:
            switch (dstNComps) {
                case 1:
                    convertToFormatInternal<SRCPIX, DSTPIX, srcMaxValue, dstMaxValue, 2, 1>(renderWindow, srcColorSpace, dstColorSpace, requiresUnpremult, conversionChannel, alphaHandling, monoConversion, srcBuf, srcBounds, dstBuf, dstBounds);
                    break;
                case 3:
                    convertToFormatInternal<SRCPIX, DSTPIX, srcMaxValue, dstMaxValue, 2, 3>(renderWindow, srcColorSpace, dstColorSpace, requiresUnpremult, conversionChannel, alphaHandling, monoConversion, srcBuf, srcBounds, dstBuf, dstBounds);
                    break;
                case 4:
                    convertToFormatInternal<SRCPIX, DSTPIX, srcMaxValue, dstMaxValue, 2, 4>(renderWindow, srcColorSpace, dstColorSpace, requiresUnpremult, conversionChannel, alphaHandling, monoConversion, srcBuf, srcBounds, dstBuf, dstBounds);
                    break;
                default:
                    assert(false);
                    break;
            }
            break;
        case 3:
            switch (dstNComps) {
                case 1:
                    convertToFormatInternal<SRCPIX, DSTPIX, srcMaxValue, dstMaxValue, 3, 1>(renderWindow, srcColorSpace, dstColorSpace, requiresUnpremult, conversionChannel, alphaHandling, monoConversion, srcBuf, srcBounds, dstBuf, dstBounds);
                    break;
                case 2:
                    convertToFormatInternal<SRCPIX, DSTPIX, srcMaxValue, dstMaxValue, 3, 2>(renderWindow, srcColorSpace, dstColorSpace, requiresUnpremult, conversionChannel, alphaHandling, monoConversion, srcBuf, srcBounds, dstBuf, dstBounds);
                    break;
                case 4:
                    convertToFormatInternal<SRCPIX, DSTPIX, srcMaxValue, dstMaxValue, 3, 4>(renderWindow, srcColorSpace, dstColorSpace, requiresUnpremult, conversionChannel, alphaHandling, monoConversion, srcBuf, srcBounds, dstBuf, dstBounds);
                    break;
                default:
                    assert(false);
                    break;
            }
            break;
        case 4:
            switch (dstNComps) {
                case 1:
                    convertToFormatInternal<SRCPIX, DSTPIX, srcMaxValue, dstMaxValue, 4, 1>(renderWindow, srcColorSpace, dstColorSpace, requiresUnpremult, conversionChannel, alphaHandling, monoConversion, srcBuf, srcBounds, dstBuf, dstBounds);
                    break;
                case 2:
                    convertToFormatInternal<SRCPIX, DSTPIX, srcMaxValue, dstMaxValue, 4, 2>(renderWindow, srcColorSpace, dstColorSpace, requiresUnpremult, conversionChannel, alphaHandling, monoConversion, srcBuf, srcBounds, dstBuf, dstBounds);
                    break;
                case 3:
                    convertToFormatInternal<SRCPIX, DSTPIX, srcMaxValue, dstMaxValue, 4, 3>(renderWindow, srcColorSpace, dstColorSpace, requiresUnpremult, conversionChannel, alphaHandling, monoConversion, srcBuf, srcBounds, dstBuf, dstBounds);
                    break;
                default:
                    assert(false);
                    break;
            }
            break;
        default:
            break;
    } // switch
} // Image::convertToFormatInternalForDepth

void
ImagePrivate::convertCPUImage(const RectI & renderWindow,
                              ViewerColorSpaceEnum srcColorSpace,
                              ViewerColorSpaceEnum dstColorSpace,
                              bool requiresUnpremult,
                              int conversionChannel,
                              Image::AlphaChannelHandlingEnum alphaHandling,
                              Image::MonoToPackedConversionEnum monoConversion,
                              const void* srcBuf,
                              int srcNComps,
                              ImageBitDepthEnum srcBitDepth,
                              const RectI& srcBounds,
                              void* dstBuf,
                              int dstNComps,
                              ImageBitDepthEnum dstBitDepth,
                              const RectI& dstBounds)
{
    assert( srcBounds.contains(renderWindow) && dstBounds.contains(renderWindow) );

    if ( dstNComps == srcNComps ) {
        switch ( dstBitDepth ) {
            case eImageBitDepthByte: {
                switch ( srcBitDepth ) {
                    case eImageBitDepthByte:
                        ///Same as a copy
                        convertToFormatInternal_sameComps<unsigned char, unsigned char, 255, 255>(renderWindow, srcColorSpace, dstColorSpace, srcBuf, srcNComps, srcBounds, dstBuf, dstBounds);
                        break;
                    case eImageBitDepthShort:
                        convertToFormatInternal_sameComps<unsigned short, unsigned char, 65535, 255>(renderWindow, srcColorSpace, dstColorSpace, srcBuf, srcNComps, srcBounds, dstBuf, dstBounds);
                        break;
                    case eImageBitDepthHalf:
                        break;
                    case eImageBitDepthFloat:
                        convertToFormatInternal_sameComps<float, unsigned char, 1, 255>(renderWindow, srcColorSpace, dstColorSpace, srcBuf, srcNComps, srcBounds, dstBuf, dstBounds);
                        break;
                    case eImageBitDepthNone:
                        break;
                }
                break;
            }

            case eImageBitDepthShort: {
                switch ( srcBitDepth ) {
                    case eImageBitDepthByte:
                        convertToFormatInternal_sameComps<unsigned char, unsigned short, 255, 65535>(renderWindow, srcColorSpace, dstColorSpace, srcBuf, srcNComps, srcBounds, dstBuf, dstBounds);
                        break;
                    case eImageBitDepthShort:
                        ///Same as a copy
                        convertToFormatInternal_sameComps<unsigned short, unsigned short, 65535, 65535>(renderWindow, srcColorSpace, dstColorSpace, srcBuf, srcNComps, srcBounds, dstBuf, dstBounds);
                        break;
                    case eImageBitDepthHalf:
                        break;
                    case eImageBitDepthFloat:
                        convertToFormatInternal_sameComps<float, unsigned short, 1, 65535>(renderWindow, srcColorSpace, dstColorSpace, srcBuf, srcNComps, srcBounds, dstBuf, dstBounds);
                        break;
                    case eImageBitDepthNone:
                        break;
                }
                break;
            }

            case eImageBitDepthHalf:
                break;

            case eImageBitDepthFloat: {
                switch ( srcBitDepth ) {
                    case eImageBitDepthByte:
                        convertToFormatInternal_sameComps<unsigned char, float, 255, 1>(renderWindow, srcColorSpace, dstColorSpace, srcBuf, srcNComps, srcBounds, dstBuf, dstBounds);
                        break;
                    case eImageBitDepthShort:
                        convertToFormatInternal_sameComps<unsigned short, float, 65535, 1>(renderWindow, srcColorSpace, dstColorSpace, srcBuf, srcNComps, srcBounds, dstBuf, dstBounds);
                        break;
                    case eImageBitDepthHalf:
                        break;
                    case eImageBitDepthFloat:
                        ///Same as a copy
                        convertToFormatInternal_sameComps<float, float, 1, 1>(renderWindow, srcColorSpace, dstColorSpace, srcBuf, srcNComps, srcBounds, dstBuf, dstBounds);
                        break;
                    case eImageBitDepthNone:
                        break;
                }
                break;
            }

            case eImageBitDepthNone:
                break;
        } // switch
    } else {
        switch ( dstBitDepth ) {
            case eImageBitDepthByte: {
                switch ( srcBitDepth ) {
                    case eImageBitDepthByte:
                        convertToFormatInternalForDepth<unsigned char, unsigned char, 255, 255>(renderWindow, srcColorSpace, dstColorSpace, requiresUnpremult, conversionChannel, alphaHandling, monoConversion, srcBuf, srcNComps, srcBounds, dstBuf, dstNComps, dstBounds);
                        break;
                    case eImageBitDepthShort:
                        convertToFormatInternalForDepth<unsigned short, unsigned char, 65535, 255>(renderWindow, srcColorSpace, dstColorSpace, requiresUnpremult, conversionChannel, alphaHandling, monoConversion, srcBuf, srcNComps, srcBounds, dstBuf, dstNComps, dstBounds);
                        break;
                    case eImageBitDepthHalf:
                        break;
                    case eImageBitDepthFloat:
                        convertToFormatInternalForDepth<float, unsigned char, 1, 255>(renderWindow, srcColorSpace, dstColorSpace, requiresUnpremult, conversionChannel, alphaHandling, monoConversion, srcBuf, srcNComps, srcBounds, dstBuf, dstNComps, dstBounds);

                        break;
                    case eImageBitDepthNone:
                        break;
                }
                break;
            }
            case eImageBitDepthShort: {
                switch ( srcBitDepth ) {
                    case eImageBitDepthByte:
                        convertToFormatInternalForDepth<unsigned char, unsigned short, 255, 65535>(renderWindow, srcColorSpace, dstColorSpace, requiresUnpremult, conversionChannel, alphaHandling, monoConversion, srcBuf, srcNComps, srcBounds, dstBuf, dstNComps, dstBounds);

                        break;
                    case eImageBitDepthShort:
                        convertToFormatInternalForDepth<unsigned short, unsigned short, 65535, 65535>(renderWindow, srcColorSpace, dstColorSpace, requiresUnpremult, conversionChannel, alphaHandling, monoConversion, srcBuf, srcNComps, srcBounds, dstBuf, dstNComps, dstBounds);

                        break;
                    case eImageBitDepthHalf:
                        break;
                    case eImageBitDepthFloat:
                        convertToFormatInternalForDepth<float, unsigned short, 1, 65535>(renderWindow, srcColorSpace, dstColorSpace, requiresUnpremult, conversionChannel, alphaHandling, monoConversion, srcBuf, srcNComps, srcBounds, dstBuf, dstNComps, dstBounds);
                        break;
                    case eImageBitDepthNone:
                        break;
                }
                break;
            }
            case eImageBitDepthHalf:
                break;
            case eImageBitDepthFloat: {
                switch ( srcBitDepth ) {
                    case eImageBitDepthByte:
                        convertToFormatInternalForDepth<unsigned char, float, 255, 1>(renderWindow, srcColorSpace, dstColorSpace, requiresUnpremult, conversionChannel, alphaHandling, monoConversion, srcBuf, srcNComps, srcBounds, dstBuf, dstNComps, dstBounds);
                        break;
                    case eImageBitDepthShort:
                        convertToFormatInternalForDepth<unsigned short, float, 65535, 1>(renderWindow, srcColorSpace, dstColorSpace, requiresUnpremult, conversionChannel, alphaHandling, monoConversion, srcBuf, srcNComps, srcBounds, dstBuf, dstNComps, dstBounds);
                        
                        break;
                    case eImageBitDepthHalf:
                        break;
                    case eImageBitDepthFloat:
                        convertToFormatInternalForDepth<float, float, 1, 1>(renderWindow, srcColorSpace, dstColorSpace, requiresUnpremult, conversionChannel, alphaHandling, monoConversion, srcBuf, srcNComps, srcBounds, dstBuf, dstNComps, dstBounds);
                        break;
                    case eImageBitDepthNone:
                        break;
                }
                break;
            }
                
        default:
            break;
        } // switch
    }
} // convertCPUImage

template <typename GL>
static void
copyGLTextureInternal(const GLCacheEntryPtr& from,
                      const GLCacheEntryPtr& to,
                      const RectI& roi,
                      const OSGLContextPtr& glContext)
{
    // Textures must belong to the same context
    assert(glContext == from->getOpenGLContext() &&
           glContext == to->getOpenGLContext());

    // Texture targets must be the same
    U32 target = from->getGLTextureTarget();
    assert(target == to->getGLTextureTarget());

    GLuint fboID = glContext->getOrCreateFBOId();
    GL::Disable(GL_SCISSOR_TEST);
    GL::BindFramebuffer(GL_FRAMEBUFFER, fboID);
    GL::Enable(target);
    GL::ActiveTexture(GL_TEXTURE0);

    U32 toTexID = to->getGLTextureID();
    U32 fromTexID = from->getGLTextureID();

    GL::BindTexture( target, toTexID );

    GL::TexParameteri (target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    GL::TexParameteri (target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    GL::TexParameteri (target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    GL::TexParameteri (target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);


    GL::FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, target, toTexID, 0 /*LoD*/);
    glCheckFramebufferError(GL);
    GL::BindTexture( target, fromTexID );

    GL::TexParameteri (target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    GL::TexParameteri (target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    GL::TexParameteri (target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    GL::TexParameteri (target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    GLShaderBasePtr shader = glContext->getOrCreateCopyTexShader();
    assert(shader);
    shader->bind();
    shader->setUniform("srcTex", 0);

    Image::applyTextureMapping<GL>(from->getBounds(), to->getBounds(), roi);

    shader->unbind();
    GL::BindTexture(target, 0);

    glCheckError(GL);
} // copyGLTextureInternal

static void
copyGLTexture(const GLCacheEntryPtr& from,
              const GLCacheEntryPtr& to,
              const RectI& roi,
              const OSGLContextPtr& glContext)
{
    if (glContext->isGPUContext()) {
        copyGLTextureInternal<GL_GPU>(from, to, roi, glContext);
    } else {
        copyGLTextureInternal<GL_CPU>(from, to, roi, glContext);
    }
}

template <typename GL>
static void
convertRGBAPackedCPUBufferToGLTextureInternal(const RAMCacheEntryPtr& buffer,
                                              const RectI& roi,
                                              const GLCacheEntryPtr& outTexture,
                                              const OSGLContextPtr& glContext)
{


    GLuint pboID = glContext->getOrCreatePBOId();
    assert(pboID != 0);

    GL::Enable(GL_TEXTURE_2D);
    // bind PBO to update texture source
    GL::BindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, pboID);

    // Only RGBA 32-bit textures for now!
    assert(outTexture->getBitDepth() == eImageBitDepthFloat && buffer->getBitDepth() == eImageBitDepthFloat);

    const std::size_t texturePixelSize = 4 * sizeof(float);
    const std::size_t pboRowBytes = roi.width() * texturePixelSize;
    const std::size_t pboDataBytes = pboRowBytes * roi.height();

    // Note that glMapBufferARB() causes sync issue.
    // If GPU is working with this buffer, glMapBufferARB() will wait(stall)
    // until GPU to finish its job. To avoid waiting (idle), you can call
    // first glBufferDataARB() with NULL pointer before glMapBufferARB().
    // If you do that, the previous data in PBO will be discarded and
    // glMapBufferARB() returns a new allocated pointer immediately
    // even if GPU is still working with the previous data.
    GL::BufferDataARB(GL_PIXEL_UNPACK_BUFFER_ARB, pboDataBytes, 0, GL_DYNAMIC_DRAW_ARB);
    unsigned char* gpuData = (unsigned char*)GL::MapBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, GL_WRITE_ONLY_ARB);

    if (!gpuData) {
        throw std::bad_alloc();
    }

    const RectI fromBounds = buffer->getBounds();
    const std::size_t srcNComps = buffer->getNumComponents();
    const std::size_t dataSizeOf = sizeof(float);
    const std::size_t srcRowBytes = buffer->getRowSize();
    const unsigned char* srcPixels = (const unsigned char*)buffer->getData();

    {
        unsigned char* dstData = gpuData;
        const unsigned char* srcRoIData = Image::pixelAtStatic(roi.x1, roi.y1, fromBounds, srcNComps, dataSizeOf, srcPixels);
        for (int y = roi.y1; y < roi.y2; ++y) {
            memcpy(dstData, srcRoIData, pboRowBytes);
            srcRoIData += srcRowBytes;
            dstData += pboRowBytes;
        }
    }


    GLboolean result = GL::UnmapBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB); // release the mapped buffer
    assert(result == GL_TRUE);
    Q_UNUSED(result);

    glCheckError(GL);

    // The creation of the image will use glTexImage2D and will get filled with the PBO


    // it is good idea to release PBOs with ID 0 after use.
    // Once bound with 0, all pixel operations are back to normal ways.
    GL::BindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, 0);
    //GL::BindTexture(GL_TEXTURE_2D, 0); // useless, we didn't bind anything
    glCheckError(GL);

}  // convertRGBAPackedCPUBufferToGLTextureInternal


static void
convertRGBAPackedCPUBufferToGLTexture(const RAMCacheEntryPtr& buffer, const RectI& roi, const GLCacheEntryPtr& outTexture)
{
    OSGLContextPtr glContext = outTexture->getOpenGLContext();

    if (glContext->isGPUContext()) {
        return convertRGBAPackedCPUBufferToGLTextureInternal<GL_GPU>(buffer, roi, outTexture, glContext);
    } else {
        return convertRGBAPackedCPUBufferToGLTextureInternal<GL_CPU>(buffer, roi, outTexture, glContext);
    }
}

void
ImagePrivate::copyRectangle(const MemoryBufferedCacheEntryBasePtr& from,
                            const int fromChannelIndex,
                            Image::ImageBufferLayoutEnum fromLayout,
                            const MemoryBufferedCacheEntryBasePtr& to,
                            const int toChannelIndex,
                            Image::ImageBufferLayoutEnum toLayout,
                            const Image::CopyPixelsArgs& args)
{
    assert(from && to);

    // Dispatch the copy function given the storage type
    GLCacheEntryPtr fromIsGLTexture = toGLCacheEntry(from);
    RAMCacheEntryPtr fromIsRAMBuffer = toRAMCacheEntry(from);
    MemoryMappedCacheEntryPtr fromIsMMAPBuffer = toMemoryMappedCacheEntry(from);

    GLCacheEntryPtr toIsGLTexture = toGLCacheEntry(to);
    RAMCacheEntryPtr toIsRAMBuffer = toRAMCacheEntry(to);
    MemoryMappedCacheEntryPtr toIsMMAPBuffer = toMemoryMappedCacheEntry(to);

    if (!fromIsGLTexture && !fromIsRAMBuffer && !fromIsMMAPBuffer) {
        // Unsupported copy scheme
        assert(false);
        return;
    }

    if (!toIsGLTexture && !toIsRAMBuffer && !toIsMMAPBuffer) {
        // Unsupported copy scheme
        assert(false);
        return;
    }

    if (fromIsGLTexture && toIsGLTexture) {
        // GL texture to GL texture
        assert(fromLayout == Image::eImageBufferLayoutRGBAPackedFullRect &&
               toLayout == Image::eImageBufferLayoutRGBAPackedFullRect);

        // OpenGL context must be the same, otherwise one of the texture should have been copied
        // to a temporary CPU image first.
        assert(fromIsGLTexture->getOpenGLContext() == toIsGLTexture->getOpenGLContext());

        copyGLTexture(fromIsGLTexture, toIsGLTexture, args.roi, fromIsGLTexture->getOpenGLContext());

    } else if (fromIsGLTexture && !toIsGLTexture) {
        // GL texture to CPU
        assert(fromLayout == Image::eImageBufferLayoutRGBAPackedFullRect &&
               toLayout == Image::eImageBufferLayoutRGBAPackedFullRect);

    } else if (!fromIsGLTexture && toIsGLTexture) {

        // CPU to GL texture
        assert(fromLayout == Image::eImageBufferLayoutRGBAPackedFullRect &&
               toLayout == Image::eImageBufferLayoutRGBAPackedFullRect);

        // The buffer can only be a RAM buffer because MMAP only supports mono channel tiles
        // which is not supported for the conversion to OpenGL textures.
        assert(fromIsRAMBuffer);
        convertRGBAPackedCPUBufferToGLTexture(fromIsRAMBuffer, args.roi, toIsGLTexture);
    } else {
        // CPU to CPU
        assert((fromIsRAMBuffer || fromIsMMAPBuffer) && (toIsRAMBuffer || toIsMMAPBuffer));

        ViewerColorSpaceEnum srcColorspace = eViewerColorSpaceLinear;
        ViewerColorSpaceEnum dstColorspace = eViewerColorSpaceLinear;
        const char* srcBuf;
        RectI srcBounds;
        ImageBitDepthEnum srcBitDepth;
        int srcNComps;
        char* dstBuf;
        RectI dstBounds;
        ImageBitDepthEnum dstBitDepth;
        int dstNComps;
        if (fromIsRAMBuffer) {
            srcBuf = fromIsRAMBuffer->getData();
            srcBounds = fromIsRAMBuffer->getBounds();
            srcBitDepth = fromIsRAMBuffer->getBitDepth();
            srcNComps = fromIsRAMBuffer->getNumComponents();
        } else {
            srcBuf = fromIsMMAPBuffer->getData();
            srcBounds = fromIsMMAPBuffer->getBounds();
            srcBitDepth = fromIsMMAPBuffer->getBitDepth();
            srcNComps = 1;
        }
        if (toIsRAMBuffer) {
            dstBuf = toIsRAMBuffer->getData();
            dstBounds = toIsRAMBuffer->getBounds();
            dstBitDepth = toIsRAMBuffer->getBitDepth();
            dstNComps = toIsRAMBuffer->getNumComponents();
        } else {
            dstBuf = toIsMMAPBuffer->getData();
            dstBounds = toIsMMAPBuffer->getBounds();
            dstBitDepth = toIsMMAPBuffer->getBitDepth();
            dstNComps = 1;
        }

        switch (fromLayout) {
            case Image::eImageBufferLayoutMonoChannelTiled: {
                switch (toLayout) {
                    case Image::eImageBufferLayoutMonoChannelTiled:
                        // Mono channel to mono channel: 1 comp in and out
                        convertCPUImage(args.roi, srcColorspace, dstColorspace, false /*unPremult*/, 0/*conversionChannel*/, Image::eAlphaChannelHandlingFillFromChannel, Image::eMonoToPackedConversionCopyToChannelAndLeaveOthers,  srcBuf, 1 /*srcNComps*/, srcBitDepth, srcBounds, dstBuf, 1 /*dstNComps*/, dstBitDepth, dstBounds);
                        break;
                    case Image::eImageBufferLayoutRGBACoplanarFullRect: {
                        assert(toChannelIndex == -1);

                        std::size_t dstRowSize = dstBounds.width() * getSizeOfForBitDepth(dstBitDepth);
                        std::size_t dstPlaneSize = dstRowSize * dstBounds.height();

                        // Writing to a co-planar buffer: need to offset the buffer
                        void* dstPixels = dstBuf + toChannelIndex * dstPlaneSize;
                        convertCPUImage(roi, srcColorspace, dstColorspace, false /*unPremult*/, 0/*conversionChannel*/, Image::eAlphaChannelHandlingFillFromChannel, Image::eMonoToPackedConversionCopyToChannelAndLeaveOthers, srcBuf, 1 /*srcNComps*/, srcBitDepth, srcBounds, dstPixels, 1 /*dstNComps*/, dstBitDepth, dstBounds);
                    }   break;
                    case Image::eImageBufferLayoutRGBAPackedFullRect:
                        assert(toChannelIndex == -1);
                        // 1 comp to N comps
                        convertCPUImage(roi, srcColorspace, dstColorspace, false /*unPremult*/, fromChannelIndex == -1 ? 0 : fromChannelIndex /*conversionChannel*/, Image::eAlphaChannelHandlingFillFromChannel, Image::eMonoToPackedConversionCopyToChannelAndLeaveOthers, srcBuf, 1 /*srcNComps*/, srcBitDepth, srcBounds, dstBuf, dstNComps, dstBitDepth, dstBounds);
                        break;
                }

            }    break;
            case Image::eImageBufferLayoutRGBACoplanarFullRect: {
                assert(fromChannelIndex == -1);
                switch (toLayout) {
                    case Image::eImageBufferLayoutMonoChannelTiled:

                        break;
                    case Image::eImageBufferLayoutRGBACoplanarFullRect:
                        assert(toChannelIndex == -1);

                        break;
                    case Image::eImageBufferLayoutRGBAPackedFullRect:
                        assert(toChannelIndex == -1);
                        
                        break;
                }

            }    break;
            case Image::eImageBufferLayoutRGBAPackedFullRect: {
                assert(fromChannelIndex == -1);
                switch (toLayout) {
                    case Image::eImageBufferLayoutMonoChannelTiled:
                        // N comps to 1 comp
                        int conversionChannel;
                        AlphaChannelHandlingEnum alphaHandling;
                        if (fromChannelIndex != -1) {
                            conversionChannel = fromChannelIndex;
                        }
                        convertCPUImage(roi, srcColorspace, dstColorspace, false /*unPremult*/, conversionChannel , alphaHandling, eMonoToPackedConversionCopyToChannelAndLeaveOthers, srcBuf, srcNComps , srcBitDepth, srcBounds, dstBuf, 1 /*dstNComps*/, dstBitDepth, dstBounds);
                        break;
                    case Image::eImageBufferLayoutRGBACoplanarFullRect:
                        assert(toChannelIndex == -1);

                        break;
                    case Image::eImageBufferLayoutRGBAPackedFullRect:
                        assert(toChannelIndex == -1);

                        break;
                }

            }    break;
        }
    }
} // copyRectangle


NATRON_NAMESPACE_EXIT;
