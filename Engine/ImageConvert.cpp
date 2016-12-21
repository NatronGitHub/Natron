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
                                  const void* srcBufPtrs[4],
                                  int nComp,
                                  const RectI& srcBounds,
                                  void* dstBufPtrs[4],
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

            const SRCPIX* srcPixelPtrs[4];
            int srcPixelStride;
            Image::getChannelPointers<SRCPIX, nComp>((const SRCPIX**)srcBufPtrs, renderWindow.x1, y, srcBounds, srcDataSizeOf, srcPixelPtrs, &srcPixelStride);

            DSTPIX* dstPixelPtrs[4];
            int dstPixelStride;
            Image::getChannelPointers<DSTPIX, nComp>((const SRCPIX**)dstBufPtrs, renderWindow.x1, y, dstBounds, dstDataSizeOf, dstPixelPtrs, &dstPixelStride);

            std::size_t nBytesToCopy = renderWindow.width() * nComp * srcDataSizeOf;
            if (nComp == 1 || !srcPixelPtrs[1]) {
                // In packed RGBA mode or single channel coplanar a single call to memcpy is needed per scan-line
                memcpy(dstPixelPtrs[0], srcPixelPtrs[0], nBytesToCopy);
            } else {
                // Ok we are in coplanar mode, copy each channel individually
                for (int c = 0; c < 4; ++c) {
                    if (srcPixelPtrs[c] && dstPixelPtrs[c]) {
                        memcpy(dstPixelPtrs[c], srcPixelPtrs[c], nBytesToCopy);
                    }
                }
            }
        } else {
            // Start of the line for error diffusion
            // coverity[dont_call]
            int start = rand() % renderWindow.width();

            const SRCPIX* srcPixelPtrs[4];
            int srcPixelStride;
            Image::getChannelPointers<SRCPIX, nComp>((const SRCPIX**)srcBufPtrs, renderWindow.x1 + start, y, srcBounds, srcDataSizeOf, srcPixelPtrs, &srcPixelStride);

            DSTPIX* dstPixelPtrs[4];
            int dstPixelStride;
            Image::getChannelPointers<DSTPIX, nComp>((const SRCPIX**)dstBufPtrs, renderWindow.x1 + start, y, dstBounds, dstDataSizeOf, dstPixelPtrs, &dstPixelStride);

            const SRCPIX* srcPixelStart[4];
            DSTPIX* dstPixelStart[4];
            memcpy(srcPixelStart, srcPixelPtrs, sizeof(SRCPIX) * 4);
            memcpy(dstPixelStart, dstPixelPtrs, sizeof(DSTPIX) * 4);
            

            for (int backward = 0; backward < 2; ++backward) {
                int x = backward ? start - 1 : start;
                int end = backward ? -1 : renderWindow.width();
                unsigned error[3] = {
                    0x80, 0x80, 0x80
                };

                while ( x != end && x >= 0 && x < renderWindow.width() ) {
                    for (int k = 0; k < nComp; ++k) {

                        if (!dstPixelPtrs[k]) {
                            continue;
                        }
                        SRCPIX sourcePixel;
                        if (srcPixelPtrs[k]) {
                            sourcePixel = *srcPixelPtrs[k];
                        } else {
                            sourcePixel = 0;
                        }
#                 ifdef DEBUG
                        assert(sourcePixel == sourcePixel); // check for NaN
#                 endif
                        DSTPIX pix;
                        if ( (k == 3) || (!srcLut && !dstLut) ) {
                            pix = Image::convertPixelDepth<SRCPIX, DSTPIX>(sourcePixel);
                        } else {
                            float pixFloat;

                            if (srcLut) {
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
                        *dstPixelPtrs[k] =  pix;
#                 ifdef DEBUG
                        assert(pix == pix); // check for NaN
#                 endif
                    }

                    if (backward) {
                        --x;
                        for (int i = 0; i < 4; ++i) {
                            if (srcPixelPtrs[i]) {
                                srcPixelPtrs[i] -= srcPixelStride;
                            }
                            if (dstPixelPtrs[i]) {
                                dstPixelPtrs[i] -= dstPixelStride;
                            }
                        }
                    } else {
                        ++x;
                        for (int i = 0; i < 4; ++i) {
                            if (srcPixelPtrs[i]) {
                                srcPixelPtrs[i] += srcPixelStride;
                            }
                            if (dstPixelPtrs[i]) {
                                dstPixelPtrs[i] += dstPixelStride;
                            }
                        }
                    }
                }
                // We went forward, now start forward, starting from the pixel before the start pixel
                for (int i = 0; i < 4; ++i) {
                    if (srcPixelPtrs[i]) {
                        srcPixelPtrs[i] = srcPixelStart[i] - srcPixelStride;
                    }
                    if (dstPixelPtrs[i]) {
                        dstPixelPtrs[i] = dstPixelStart[i] - dstPixelStride;
                    }
                }
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
                                            Image::AlphaChannelHandlingEnum alphaHandling,
                                            const void* srcBufPtrs[4],
                                            const RectI& srcBounds,
                                            void* dstBufPtrs[4],
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

        const SRCPIX* srcPixelPtrs[4];
        int srcPixelStride;
        Image::getChannelPointers<SRCPIX, srcNComps>((const SRCPIX**)srcBufPtrs, renderWindow.x1 + start, y, srcBounds, srcDataSizeOf, srcPixelPtrs, &srcPixelStride);

        DSTPIX* dstPixelPtrs[4];
        int dstPixelStride;
        Image::getChannelPointers<DSTPIX, dstNComps>((const SRCPIX**)dstBufPtrs, renderWindow.x1 + start, y, dstBounds, dstDataSizeOf, dstPixelPtrs, &dstPixelStride);

        const SRCPIX* srcPixelStart[4];
        DSTPIX* dstPixelStart[4];
        memcpy(srcPixelStart, srcPixelPtrs, sizeof(SRCPIX) * 4);
        memcpy(dstPixelStart, dstPixelPtrs, sizeof(DSTPIX) * 4);


        // We do twice the loop, once from starting point to end and once from starting point - 1 to real start
        for (int backward = 0; backward < 2; ++backward) {

            int x = backward ? start - 1 : start;

            // End is pointing to the first pixel outside the line a la stl
            int end = backward ? -1 : renderWindow.width();

            // The error will be updated and diffused throughout the scanline
            unsigned error[3] = {
                0x80, 0x80, 0x80
            };

            while (x != end) {

                // We've XY, RGB or RGBA input and outputs
                assert(srcNComps != dstNComps);

                const bool unpremultChannel = ( //srcNComps == 4 && // test already done in convertToFormatInternalForDepth
                                               //dstNComps == 3 && // test already done in convertToFormatInternalForDepth
                                               requiresUnpremult);

                // This is only set if unpremultChannel is true
                float alphaForUnPremult;
                if (unpremultChannel && srcPixelPtrs[3]) {
                    alphaForUnPremult = Image::convertPixelDepth<SRCPIX, float>(*srcPixelPtrs[3]);
                } else {
                    alphaForUnPremult = 1.;
                }

                // For RGB channels, unpremult and do colorspace conversion if needed.
                // For all channels, converting pixel depths is required at the very least.
                for (int k = 0; k < 3 && k < dstNComps; ++k) {

                    if (!dstPixelPtrs[k]) {
                        continue;
                    }
                    SRCPIX sourcePixel;

                    if (srcPixelPtrs[k]) {
                        sourcePixel = *srcPixelPtrs[k];
                    } else {
                        sourcePixel = 0;
                    }

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
                    *dstPixelPtrs[k] =  pix;
#                 ifdef DEBUG
                    assert(*dstPixelPtrs[k] == *dstPixelPtrs[k]); // check for NaN
#                 endif


                } // for (int k = 0; k < k < 3 && k < dstNComps; ++k) {

                if (dstPixelPtrs[3] && !srcPixelPtrs[3]) {
                    // we reach here only if converting RGB-->RGBA or XY--->RGBA
                    DSTPIX pix;
                    switch (alphaHandling) {
                        case Image::eAlphaChannelHandlingCreateFill0:
                            pix = 0;
                            break;
                        case Image::eAlphaChannelHandlingCreateFill1:
                            pix = (DSTPIX)dstMaxValue;
                            break;
                        case Image::eAlphaChannelHandlingFillFromChannel:
                            assert(conversionChannel >= 0 && conversionChannel < srcNComps);
                            if (srcPixelPtrs[conversionChannel]) {
                                pix = Image::convertPixelDepth<SRCPIX, DSTPIX>(*srcPixelPtrs[conversionChannel]);
                            } else {
                                pix = (DSTPIX)dstMaxValue;
                            }
                            break;
                    }

                    *dstPixelPtrs[3] = pix;
                }
                if (backward) {
                    --x;
                    for (int i = 0; i < 4; ++i) {
                        if (srcPixelPtrs[i]) {
                            srcPixelPtrs[i] -= srcPixelStride;
                        }
                        if (dstPixelPtrs[i]) {
                            dstPixelPtrs[i] -= dstPixelStride;
                        }
                    }
                } else {
                    ++x;
                    for (int i = 0; i < 4; ++i) {
                        if (srcPixelPtrs[i]) {
                            srcPixelPtrs[i] += srcPixelStride;
                        }
                        if (dstPixelPtrs[i]) {
                            dstPixelPtrs[i] += dstPixelStride;
                        }
                    }
                }
            } // for all pixels in the scan-line

            // We went forward, now start forward, starting from the pixel before the start pixel
            for (int i = 0; i < 4; ++i) {
                if (srcPixelPtrs[i]) {
                    srcPixelPtrs[i] = srcPixelStart[i] - srcPixelStride;
                }
                if (dstPixelPtrs[i]) {
                    dstPixelPtrs[i] = dstPixelStart[i] - dstPixelStride;
                }
            }
        } // for (int backward = 0; backward < 2; ++backward) {
    }  // for (int y = 0; y < renderWindow.height(); ++y) {

} // convertToFormatInternalForColorSpace

template <typename SRCPIX, typename DSTPIX, int srcMaxValue, int dstMaxValue, int srcNComps, Image::AlphaChannelHandlingEnum alphaHandling>
void
static convertToMonoImage(const RectI & renderWindow,
                          int conversionChannel,
                          const void* srcBufPtrs[4],
                          const RectI& srcBounds,
                          void* dstBufPtrs[4],
                          const RectI& dstBounds)
{
    assert(dstBufPtrs[0] && !dstBufPtrs[1] && !dstBufPtrs[2] && !dstBufPtrs[3]);

    int dstDataSizeOf = sizeof(DSTPIX);
    int srcDataSizeOf = sizeof(SRCPIX);

    DSTPIX pix;

    for (int y = renderWindow.y1; y < renderWindow.y2; ++y) {
        // Start of the line for error diffusion
        // coverity[dont_call]

        const SRCPIX* srcPixelPtrs[4];
        int srcPixelStride;
        Image::getChannelPointers<SRCPIX, srcNComps>((const SRCPIX**)srcBufPtrs, renderWindow.x1, y, srcBounds, srcDataSizeOf, srcPixelPtrs, &srcPixelStride);

        DSTPIX* dstPixelPtrs[4];
        int dstPixelStride;
        Image::getChannelPointers<DSTPIX, 1>((const SRCPIX**)dstBufPtrs, renderWindow.x1, y, dstBounds, dstDataSizeOf, dstPixelPtrs, &dstPixelStride);


        for (int x = renderWindow.x1; x < renderWindow.x2; ++x) {
            switch (alphaHandling) {
                case Image::eAlphaChannelHandlingCreateFill0:
                    pix = 0;
                    break;
                case Image::eAlphaChannelHandlingCreateFill1:
                    pix = (DSTPIX)dstMaxValue;
                    break;
                case Image::eAlphaChannelHandlingFillFromChannel:
                    assert(conversionChannel >= 0 && conversionChannel < srcNComps);
                    Image::convertPixelDepth<SRCPIX, DSTPIX>(*srcPixelPtrs[conversionChannel]);
                    // Only increment the conversion channel pointer, this is the only one we use
                    srcPixelPtrs[conversionChannel] += srcPixelStride;
                    break;
                    
            }
            *dstPixelPtrs[0] = pix;
            dstPixelPtrs[0] += dstPixelStride;
        }
    }
} // convertToMonoImage

template <typename SRCPIX, typename DSTPIX, int srcMaxValue, int dstMaxValue, int dstNComps, Image::MonoToPackedConversionEnum monoConversion>
void
static convertFromMonoImage(const RectI & renderWindow,
                          int conversionChannel,
                          const void* srcBufPtrs[4],
                          const RectI& srcBounds,
                          void* dstBufPtrs[4],
                          const RectI& dstBounds)
{
    int dstDataSizeOf = sizeof(DSTPIX);
    int srcDataSizeOf = sizeof(SRCPIX);

    assert(srcBufPtrs[0] && !srcBufPtrs[1] && !srcBufPtrs[2] && !srcBufPtrs[3]);

    DSTPIX pix;

    for (int y = renderWindow.y1; y < renderWindow.y2; ++y) {
        // Start of the line for error diffusion
        // coverity[dont_call]

        const SRCPIX* srcPixelPtrs[4];
        int srcPixelStride;
        Image::getChannelPointers<SRCPIX, 1>((const SRCPIX**)srcBufPtrs, renderWindow.x1, y, srcBounds, srcDataSizeOf, srcPixelPtrs, &srcPixelStride);

        DSTPIX* dstPixelPtrs[4];
        int dstPixelStride;
        Image::getChannelPointers<DSTPIX, dstNComps>((const SRCPIX**)dstBufPtrs, renderWindow.x1, y, dstBounds, dstDataSizeOf, dstPixelPtrs, &dstPixelStride);

        for (int x = renderWindow.x1; x < renderWindow.x2; ++x) {

            pix = Image::convertPixelDepth<SRCPIX, DSTPIX>(*srcPixelPtrs[0]);
            srcPixelPtrs[0] += srcPixelStride;

            switch (monoConversion) {
                case Image::eMonoToPackedConversionCopyToAll: {
                    for (int c = 0; c < dstNComps; ++c) {
                        *dstPixelPtrs[c] = pix;
                        dstPixelPtrs[c] += dstPixelStride;
                    }
                }   break;
                case Image::eMonoToPackedConversionCopyToChannelAndFillOthers: {
                    assert(conversionChannel >= 0 && conversionChannel < dstNComps);
                    for (int c = 0; c < dstNComps; ++c) {
                        if (c == conversionChannel) {
                            *dstPixelPtrs[c] = pix;
                        } else {
                            // Fill
                            *dstPixelPtrs[c] = 0;
                        }
                        dstPixelPtrs[c] += dstPixelStride;
                    }

                }   break;
                case Image::eMonoToPackedConversionCopyToChannelAndLeaveOthers:
                    *dstPixelPtrs[conversionChannel] = pix;

                    // Don't increment other dst pixel pointers as we don't use them.
                    dstPixelPtrs[conversionChannel] += dstPixelStride;
                    break;

            }
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
                                    const void* srcBufPtrs[4],
                                    const RectI& srcBounds,
                                    void* dstBufPtrs[4],
                                    const RectI& dstBounds)
{
    if ( (srcColorSpace == eViewerColorSpaceLinear) && (dstColorSpace == eViewerColorSpaceLinear) ) {
        convertToFormatInternalForColorSpace<SRCPIX, DSTPIX, srcMaxValue, dstMaxValue, srcNComps, dstNComps, requiresUnpremult, false>(renderWindow, srcColorSpace, dstColorSpace, conversionChannel, alphaHandling, srcBufPtrs, srcBounds, dstBufPtrs, dstBounds);
    } else {
        convertToFormatInternalForColorSpace<SRCPIX, DSTPIX, srcMaxValue, dstMaxValue, srcNComps, dstNComps, requiresUnpremult, true>(renderWindow, srcColorSpace, dstColorSpace, conversionChannel, alphaHandling, srcBufPtrs, srcBounds, dstBufPtrs, dstBounds);
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
                        const void* srcBufPtrs[4],
                        const RectI& srcBounds,
                        void* dstBufPtrs[4],
                        const RectI& dstBounds)
{
    if (dstNComps == 1) {
        // When converting to alpha, do not colorspace conversion
        // optimize the conversion
        switch (alphaHandling) {
            case Image::eAlphaChannelHandlingFillFromChannel:
                convertToMonoImage<SRCPIX, DSTPIX, srcMaxValue, dstMaxValue, srcNComps, Image::eAlphaChannelHandlingFillFromChannel>(renderWindow, conversionChannel, srcBufPtrs, srcBounds, dstBufPtrs, dstBounds);
                break;
            case Image::eAlphaChannelHandlingCreateFill1:
                convertToMonoImage<SRCPIX, DSTPIX, srcMaxValue, dstMaxValue, srcNComps, Image::eAlphaChannelHandlingCreateFill1>(renderWindow, conversionChannel, srcBufPtrs, srcBounds, dstBufPtrs, dstBounds);
                break;
            case Image::eAlphaChannelHandlingCreateFill0:
                convertToMonoImage<SRCPIX, DSTPIX, srcMaxValue, dstMaxValue, srcNComps, Image::eAlphaChannelHandlingCreateFill0>(renderWindow, conversionChannel, srcBufPtrs, srcBounds, dstBufPtrs, dstBounds);
                break;
        }
    } else if (srcNComps == 1) {
        // Mono image to something else, optimize
        switch (monoConversion) {
            case Image::eMonoToPackedConversionCopyToChannelAndLeaveOthers:
                convertFromMonoImage<SRCPIX, DSTPIX, srcMaxValue, dstMaxValue, srcNComps, Image::eMonoToPackedConversionCopyToChannelAndLeaveOthers>(renderWindow, conversionChannel, srcBufPtrs, srcBounds, dstBufPtrs, dstBounds);
                break;
            case Image::eMonoToPackedConversionCopyToChannelAndFillOthers:
                convertFromMonoImage<SRCPIX, DSTPIX, srcMaxValue, dstMaxValue, srcNComps, Image::eMonoToPackedConversionCopyToChannelAndFillOthers>(renderWindow, conversionChannel, srcBufPtrs, srcBounds, dstBufPtrs, dstBounds);
                break;
            case Image::eMonoToPackedConversionCopyToAll:
                convertFromMonoImage<SRCPIX, DSTPIX, srcMaxValue, dstMaxValue, srcNComps, Image::eMonoToPackedConversionCopyToAll>(renderWindow, conversionChannel, srcBufPtrs, srcBounds, dstBufPtrs, dstBounds);
                break;
        }
    } else {
        // General case
        if (requiresUnpremult) {
            convertToFormatInternalForUnpremult<SRCPIX, DSTPIX, srcMaxValue, dstMaxValue, srcNComps, dstNComps, true>(renderWindow, srcColorSpace, dstColorSpace, conversionChannel, alphaHandling, srcBufPtrs, srcBounds, dstBufPtrs, dstBounds);
        } else {
            convertToFormatInternalForUnpremult<SRCPIX, DSTPIX, srcMaxValue, dstMaxValue, srcNComps, dstNComps, false>(renderWindow, srcColorSpace, dstColorSpace, conversionChannel, alphaHandling, srcBufPtrs, srcBounds, dstBufPtrs, dstBounds);
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
                                const void* srcBufPtrs[4],
                                int srcNComps,
                                const RectI& srcBounds,
                                void* dstBufPtrs[4],
                                int dstNComps,
                                const RectI& dstBounds)
{

    // see convertToFormatInternalForColorSpace : it is only used in one case!
    assert(!requiresUnpremult || (srcNComps == 4 && dstNComps == 3));

    switch (srcNComps) {
        case 1:
            switch (dstNComps) {
                case 2:
                    convertToFormatInternal<SRCPIX, DSTPIX, srcMaxValue, dstMaxValue, 1, 2>(renderWindow, srcColorSpace, dstColorSpace, requiresUnpremult, conversionChannel, alphaHandling, monoConversion, srcBufPtrs, srcBounds, dstBufPtrs, dstBounds);
                    break;
                case 3:
                    convertToFormatInternal<SRCPIX, DSTPIX, srcMaxValue, dstMaxValue, 1, 3>(renderWindow, srcColorSpace, dstColorSpace, requiresUnpremult, conversionChannel, alphaHandling, monoConversion, srcBufPtrs, srcBounds, dstBufPtrs, dstBounds);
                    break;
                case 4:
                    convertToFormatInternal<SRCPIX, DSTPIX, srcMaxValue, dstMaxValue, 1, 4>(renderWindow, srcColorSpace, dstColorSpace, requiresUnpremult, conversionChannel, alphaHandling, monoConversion, srcBufPtrs, srcBounds, dstBufPtrs, dstBounds);
                    break;
                default:
                    assert(false);
                    break;
            }
            break;
        case 2:
            switch (dstNComps) {
                case 1:
                    convertToFormatInternal<SRCPIX, DSTPIX, srcMaxValue, dstMaxValue, 2, 1>(renderWindow, srcColorSpace, dstColorSpace, requiresUnpremult, conversionChannel, alphaHandling, monoConversion, srcBufPtrs, srcBounds, dstBufPtrs, dstBounds);
                    break;
                case 3:
                    convertToFormatInternal<SRCPIX, DSTPIX, srcMaxValue, dstMaxValue, 2, 3>(renderWindow, srcColorSpace, dstColorSpace, requiresUnpremult, conversionChannel, alphaHandling, monoConversion, srcBufPtrs, srcBounds, dstBufPtrs, dstBounds);
                    break;
                case 4:
                    convertToFormatInternal<SRCPIX, DSTPIX, srcMaxValue, dstMaxValue, 2, 4>(renderWindow, srcColorSpace, dstColorSpace, requiresUnpremult, conversionChannel, alphaHandling, monoConversion, srcBufPtrs, srcBounds, dstBufPtrs, dstBounds);
                    break;
                default:
                    assert(false);
                    break;
            }
            break;
        case 3:
            switch (dstNComps) {
                case 1:
                    convertToFormatInternal<SRCPIX, DSTPIX, srcMaxValue, dstMaxValue, 3, 1>(renderWindow, srcColorSpace, dstColorSpace, requiresUnpremult, conversionChannel, alphaHandling, monoConversion, srcBufPtrs, srcBounds, dstBufPtrs, dstBounds);
                    break;
                case 2:
                    convertToFormatInternal<SRCPIX, DSTPIX, srcMaxValue, dstMaxValue, 3, 2>(renderWindow, srcColorSpace, dstColorSpace, requiresUnpremult, conversionChannel, alphaHandling, monoConversion, srcBufPtrs, srcBounds, dstBufPtrs, dstBounds);
                    break;
                case 4:
                    convertToFormatInternal<SRCPIX, DSTPIX, srcMaxValue, dstMaxValue, 3, 4>(renderWindow, srcColorSpace, dstColorSpace, requiresUnpremult, conversionChannel, alphaHandling, monoConversion, srcBufPtrs, srcBounds, dstBufPtrs, dstBounds);
                    break;
                default:
                    assert(false);
                    break;
            }
            break;
        case 4:
            switch (dstNComps) {
                case 1:
                    convertToFormatInternal<SRCPIX, DSTPIX, srcMaxValue, dstMaxValue, 4, 1>(renderWindow, srcColorSpace, dstColorSpace, requiresUnpremult, conversionChannel, alphaHandling, monoConversion, srcBufPtrs, srcBounds, dstBufPtrs, dstBounds);
                    break;
                case 2:
                    convertToFormatInternal<SRCPIX, DSTPIX, srcMaxValue, dstMaxValue, 4, 2>(renderWindow, srcColorSpace, dstColorSpace, requiresUnpremult, conversionChannel, alphaHandling, monoConversion, srcBufPtrs, srcBounds, dstBufPtrs, dstBounds);
                    break;
                case 3:
                    convertToFormatInternal<SRCPIX, DSTPIX, srcMaxValue, dstMaxValue, 4, 3>(renderWindow, srcColorSpace, dstColorSpace, requiresUnpremult, conversionChannel, alphaHandling, monoConversion, srcBufPtrs, srcBounds, dstBufPtrs, dstBounds);
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
                              const void* srcBufPtrs[4],
                              int srcNComps,
                              ImageBitDepthEnum srcBitDepth,
                              const RectI& srcBounds,
                              void* dstBufPtrs[4],
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
                        convertToFormatInternal_sameComps<unsigned char, unsigned char, 255, 255>(renderWindow, srcColorSpace, dstColorSpace, srcBufPtrs, srcNComps, srcBounds, dstBufPtrs, dstBounds);
                        break;
                    case eImageBitDepthShort:
                        convertToFormatInternal_sameComps<unsigned short, unsigned char, 65535, 255>(renderWindow, srcColorSpace, dstColorSpace, srcBufPtrs, srcNComps, srcBounds, dstBufPtrs, dstBounds);
                        break;
                    case eImageBitDepthHalf:
                        break;
                    case eImageBitDepthFloat:
                        convertToFormatInternal_sameComps<float, unsigned char, 1, 255>(renderWindow, srcColorSpace, dstColorSpace, srcBufPtrs, srcNComps, srcBounds, dstBufPtrs, dstBounds);
                        break;
                    case eImageBitDepthNone:
                        break;
                }
                break;
            }

            case eImageBitDepthShort: {
                switch ( srcBitDepth ) {
                    case eImageBitDepthByte:
                        convertToFormatInternal_sameComps<unsigned char, unsigned short, 255, 65535>(renderWindow, srcColorSpace, dstColorSpace, srcBufPtrs, srcNComps, srcBounds, dstBufPtrs, dstBounds);
                        break;
                    case eImageBitDepthShort:
                        ///Same as a copy
                        convertToFormatInternal_sameComps<unsigned short, unsigned short, 65535, 65535>(renderWindow, srcColorSpace, dstColorSpace, srcBufPtrs, srcNComps, srcBounds, dstBufPtrs, dstBounds);
                        break;
                    case eImageBitDepthHalf:
                        break;
                    case eImageBitDepthFloat:
                        convertToFormatInternal_sameComps<float, unsigned short, 1, 65535>(renderWindow, srcColorSpace, dstColorSpace, srcBufPtrs, srcNComps, srcBounds, dstBufPtrs, dstBounds);
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
                        convertToFormatInternal_sameComps<unsigned char, float, 255, 1>(renderWindow, srcColorSpace, dstColorSpace, srcBufPtrs, srcNComps, srcBounds, dstBufPtrs, dstBounds);
                        break;
                    case eImageBitDepthShort:
                        convertToFormatInternal_sameComps<unsigned short, float, 65535, 1>(renderWindow, srcColorSpace, dstColorSpace, srcBufPtrs, srcNComps, srcBounds, dstBufPtrs, dstBounds);
                        break;
                    case eImageBitDepthHalf:
                        break;
                    case eImageBitDepthFloat:
                        ///Same as a copy
                        convertToFormatInternal_sameComps<float, float, 1, 1>(renderWindow, srcColorSpace, dstColorSpace, srcBufPtrs, srcNComps, srcBounds, dstBufPtrs, dstBounds);
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
                        convertToFormatInternalForDepth<unsigned char, unsigned char, 255, 255>(renderWindow, srcColorSpace, dstColorSpace, requiresUnpremult, conversionChannel, alphaHandling, monoConversion, srcBufPtrs, srcNComps, srcBounds, dstBufPtrs, dstNComps, dstBounds);
                        break;
                    case eImageBitDepthShort:
                        convertToFormatInternalForDepth<unsigned short, unsigned char, 65535, 255>(renderWindow, srcColorSpace, dstColorSpace, requiresUnpremult, conversionChannel, alphaHandling, monoConversion, srcBufPtrs, srcNComps, srcBounds, dstBufPtrs, dstNComps, dstBounds);
                        break;
                    case eImageBitDepthHalf:
                        break;
                    case eImageBitDepthFloat:
                        convertToFormatInternalForDepth<float, unsigned char, 1, 255>(renderWindow, srcColorSpace, dstColorSpace, requiresUnpremult, conversionChannel, alphaHandling, monoConversion, srcBufPtrs, srcNComps, srcBounds, dstBufPtrs, dstNComps, dstBounds);

                        break;
                    case eImageBitDepthNone:
                        break;
                }
                break;
            }
            case eImageBitDepthShort: {
                switch ( srcBitDepth ) {
                    case eImageBitDepthByte:
                        convertToFormatInternalForDepth<unsigned char, unsigned short, 255, 65535>(renderWindow, srcColorSpace, dstColorSpace, requiresUnpremult, conversionChannel, alphaHandling, monoConversion, srcBufPtrs, srcNComps, srcBounds, dstBufPtrs, dstNComps, dstBounds);

                        break;
                    case eImageBitDepthShort:
                        convertToFormatInternalForDepth<unsigned short, unsigned short, 65535, 65535>(renderWindow, srcColorSpace, dstColorSpace, requiresUnpremult, conversionChannel, alphaHandling, monoConversion, srcBufPtrs, srcNComps, srcBounds, dstBufPtrs, dstNComps, dstBounds);

                        break;
                    case eImageBitDepthHalf:
                        break;
                    case eImageBitDepthFloat:
                        convertToFormatInternalForDepth<float, unsigned short, 1, 65535>(renderWindow, srcColorSpace, dstColorSpace, requiresUnpremult, conversionChannel, alphaHandling, monoConversion, srcBufPtrs, srcNComps, srcBounds, dstBufPtrs, dstNComps, dstBounds);
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
                        convertToFormatInternalForDepth<unsigned char, float, 255, 1>(renderWindow, srcColorSpace, dstColorSpace, requiresUnpremult, conversionChannel, alphaHandling, monoConversion, srcBufPtrs, srcNComps, srcBounds, dstBufPtrs, dstNComps, dstBounds);
                        break;
                    case eImageBitDepthShort:
                        convertToFormatInternalForDepth<unsigned short, float, 65535, 1>(renderWindow, srcColorSpace, dstColorSpace, requiresUnpremult, conversionChannel, alphaHandling, monoConversion, srcBufPtrs, srcNComps, srcBounds, dstBufPtrs, dstNComps, dstBounds);
                        
                        break;
                    case eImageBitDepthHalf:
                        break;
                    case eImageBitDepthFloat:
                        convertToFormatInternalForDepth<float, float, 1, 1>(renderWindow, srcColorSpace, dstColorSpace, requiresUnpremult, conversionChannel, alphaHandling, monoConversion, srcBufPtrs, srcNComps, srcBounds, dstBufPtrs, dstNComps, dstBounds);
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
    // The OpenGL context must be current to this thread.
    assert(appPTR->getGPUContextPool()->getThreadLocalContext()->getContext() == glContext);

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

    // The OpenGL context must be current to this thread.
    assert(appPTR->getGPUContextPool()->getThreadLocalContext()->getContext() == glContext);
    assert(glContext == outTexture->getOpenGLContext());
    
    // This function only supports RGBA float input buffer
    assert(buffer->getNumComponents() == 4 && buffer->getBitDepth() == eImageBitDepthFloat);;

    // Only RGBA 32-bit textures for now
    assert(outTexture->getBitDepth() == eImageBitDepthFloat);

    GLuint pboID = glContext->getOrCreatePBOId();
    assert(pboID != 0);

    GL::Enable(GL_TEXTURE_2D);
    // bind PBO to update texture source
    GL::BindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, pboID);

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

    // Copy the CPU buffer to the PBO
    {
        const RectI fromBounds = buffer->getBounds();
        const std::size_t srcNComps = buffer->getNumComponents();
        const std::size_t dataSizeOf = getSizeOfForBitDepth(buffer->getBitDepth());
        const std::size_t srcRowBytes = buffer->getRowSize();
        const unsigned char* srcBuffer = (const unsigned char*)buffer->getData();

        {
            unsigned char* dstData = gpuData;
            const unsigned char* srcPixels = Image::pixelAtStatic(roi.x1, roi.y1, fromBounds, srcNComps, dataSizeOf, srcBuffer);
            for (int y = roi.y1; y < roi.y2; ++y) {
                memcpy(dstData, srcPixels, pboRowBytes);
                srcPixels += srcRowBytes;
                dstData += pboRowBytes;
            }
        }


        GLboolean result = GL::UnmapBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB); // release the mapped buffer
        assert(result == GL_TRUE);
        Q_UNUSED(result);
        
        glCheckError(GL);
    }

    U32 target = outTexture->getGLTextureTarget();
    U32 outTextureID = outTexture->getGLTextureID();

    // bind the texture
    GL::BindTexture( target, outTextureID );

    // copy pixels from PBO to texture object
    // Use offset instead of pointer (last parameter is 0).
    GL::TexSubImage2D(target,
                      0,              // level
                      roi.x1, roi.y1,               // xoffset, yoffset
                      roi.width(), roi.height(),
                      outTexture->getGLTextureFormat(),            // format
                      outTexture->getGLTextureType(),       // type
                      0);

    GL::BindTexture(target, 0);


    // it is good idea to release PBOs with ID 0 after use.
    // Once bound with 0, all pixel operations are back to normal ways.
    GL::BindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, 0);
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

template <typename GL>
static void
convertGLTextureToRGBAPackedCPUBufferInternal(const GLCacheEntryPtr& texture,
                                              const RectI& roi,
                                              const RAMCacheEntryPtr& outBuffer,
                                              const OSGLContextPtr& glContext)
{
    // The OpenGL context must be current to this thread.
    assert(appPTR->getGPUContextPool()->getThreadLocalContext()->getContext() == glContext);
    assert(glContext == texture->getOpenGLContext());

    // This function only supports reading to a RGBA float buffer.
    assert(outBuffer->getNumComponents() == 4 && outBuffer->getBitDepth() == eImageBitDepthFloat);

    GLuint fboID = glContext->getOrCreateFBOId();

    int target = texture->getGLTextureTarget();
    U32 texID = texture->getGLTextureID();
    RectI texBounds = texture->getBounds();

    // The texture must be read in a buffer with the same bounds
    assert(outBuffer->getBounds() == texBounds);

    GL::BindFramebuffer(GL_FRAMEBUFFER, fboID);
    GL::Enable(target);
    GL::BindTexture( target, texID );
    GL::FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, target, texID, 0 /*LoD*/);

    GL::Viewport( roi.x1 - texBounds.x1, roi.y1 - texBounds.y1, roi.width(), roi.height() );
    glCheckFramebufferError(GL);

    // Ensure all drawing commands are finished
    GL::Flush();
    GL::Finish();
    glCheckError(GL);

    // Transfer the texture to the CPU buffer
    {
        unsigned char* data = Image::pixelAtStatic(roi.x1, roi.y1, texBounds, outBuffer->getNumComponents(), getSizeOfForBitDepth(outBuffer->getBitDepth()), (unsigned char*)outBuffer->getData());
        GL::ReadPixels(roi.x1 - texBounds.x1, roi.y1 - texBounds.y1, roi.width(), roi.height(), texture->getGLTextureFormat(), texture->getGLTextureType(), (GLvoid*)data);
    }

    GL::BindTexture(target, 0);
    GL::BindFramebuffer(GL_FRAMEBUFFER, 0);
    glCheckError(GL);

} // convertGLTextureToRGBAPackedCPUBufferInternal

static void
convertGLTextureToRGBAPackedCPUBuffer(const GLCacheEntryPtr& texture,
                                      const RectI& roi,
                                      const RAMCacheEntryPtr& outBuffer,
                                      const OSGLContextPtr& glContext)
{
    if (glContext->isGPUContext()) {
        convertGLTextureToRGBAPackedCPUBufferInternal<GL_GPU>(texture, roi, outBuffer, glContext);
    } else {
        convertGLTextureToRGBAPackedCPUBufferInternal<GL_CPU>(texture, roi, outBuffer, glContext);
    }
}

void
ImagePrivate::copyRectangle(const ImageTile& fromTile,
                            StorageModeEnum fromStorage,
                            Image::ImageBufferLayoutEnum fromLayout,
                            const ImageTile& toTile,
                            StorageModeEnum toStorage,
                            Image::ImageBufferLayoutEnum toLayout,
                            const Image::CopyPixelsArgs& args)
{
    assert(fromStorage != eStorageModeNone && toStorage != eStorageModeNone);

    if (fromStorage == eStorageModeGLTex && toStorage == eStorageModeGLTex) {
        // GL texture to GL texture
        assert(fromLayout == Image::eImageBufferLayoutRGBAPackedFullRect &&
               toLayout == Image::eImageBufferLayoutRGBAPackedFullRect);

        GLCacheEntryPtr fromIsGLTexture = toGLCacheEntry(fromTile.perChannelTile[0].buffer);
        GLCacheEntryPtr toIsGLTexture = toGLCacheEntry(toTile.perChannelTile[0].buffer);

        assert(fromIsGLTexture && toIsGLTexture);
        // OpenGL context must be the same, otherwise one of the texture should have been copied
        // to a temporary CPU image first.
        assert(fromIsGLTexture->getOpenGLContext() == toIsGLTexture->getOpenGLContext());

        // Save the current context
        OSGLContextSaver saveCurrentContext;

        {
            // Ensure this context is attached
            OSGLContextAttacherPtr contextAttacher = OSGLContextAttacher::create(fromIsGLTexture->getOpenGLContext());
            contextAttacher->attach();
            copyGLTexture(fromIsGLTexture, toIsGLTexture, args.roi, fromIsGLTexture->getOpenGLContext());
        }

    } else if (fromStorage == eStorageModeGLTex && toStorage != eStorageModeGLTex) {
        // GL texture to CPU
        assert(fromLayout == Image::eImageBufferLayoutRGBAPackedFullRect &&
               toLayout == Image::eImageBufferLayoutRGBAPackedFullRect);

        GLCacheEntryPtr fromIsGLTexture = toGLCacheEntry(fromTile.perChannelTile[0].buffer);
        RAMCacheEntryPtr toIsRAMBuffer = toRAMCacheEntry(toTile.perChannelTile[0].buffer);

        // The buffer can only be a RAM buffer because MMAP only supports mono channel tiles
        // which is not supported for the conversion to OpenGL textures.
        assert(fromIsGLTexture && toIsRAMBuffer);
        assert(toLayout == Image::eImageBufferLayoutRGBAPackedFullRect && toIsRAMBuffer->getNumComponents() == 4);

        // Save the current context
        OSGLContextSaver saveCurrentContext;

        {
            // Ensure this context is attached
            OSGLContextAttacherPtr contextAttacher = OSGLContextAttacher::create(fromIsGLTexture->getOpenGLContext());
            contextAttacher->attach();
            convertGLTextureToRGBAPackedCPUBuffer(fromIsGLTexture, args.roi, toIsRAMBuffer, contextAttacher->getContext());
        }

    } else if (fromStorage != eStorageModeGLTex && toStorage == eStorageModeGLTex) {

        // CPU to GL texture
        assert(fromLayout == Image::eImageBufferLayoutRGBAPackedFullRect &&
               toLayout == Image::eImageBufferLayoutRGBAPackedFullRect);

        GLCacheEntryPtr toIsGLTexture = toGLCacheEntry(toTile.perChannelTile[0].buffer);
        RAMCacheEntryPtr fromIsRAMBuffer = toRAMCacheEntry(fromTile.perChannelTile[0].buffer);

        // The buffer can only be a RAM buffer because MMAP only supports mono channel tiles
        // which is not supported for the conversion to OpenGL textures.
        assert(toIsGLTexture && fromIsRAMBuffer);
        assert(fromLayout == Image::eImageBufferLayoutRGBAPackedFullRect && fromIsRAMBuffer->getNumComponents() == 4);

        // Save the current context
        OSGLContextSaver saveCurrentContext;

        {
            // Ensure this context is attached
            OSGLContextAttacherPtr contextAttacher = OSGLContextAttacher::create(toIsGLTexture->getOpenGLContext());
            contextAttacher->attach();

            convertRGBAPackedCPUBufferToGLTexture(fromIsRAMBuffer, args.roi, toIsGLTexture);
        }
    } else {

        // CPU to CPU

        // The pointer to the RGBA channels.
        // If layout is RGBAPacked only the first pointer is valid and points to the RGBA buffer.
        // If layout is RGBACoplanar or mono channel tiled all pointers are set (if they exist).
        const void* srcPtrs[4] = {0, 0, 0, 0};
        RectI srcBounds;
        ImageBitDepthEnum srcBitDepth = eImageBitDepthNone;
        int srcNComps = 0;
        void* dstPtrs[4] = {0, 0, 0, 0};
        RectI dstBounds;
        ImageBitDepthEnum dstBitDepth = eImageBitDepthNone;
        int dstNComps = 0;

        for (std::size_t i = 0; i < fromTile.perChannelTile.size(); ++i) {
            RAMCacheEntryPtr fromIsRAMBuffer = toRAMCacheEntry(fromTile.perChannelTile[i].buffer);
            MemoryMappedCacheEntryPtr fromIsMMAPBuffer = toMemoryMappedCacheEntry(fromTile.perChannelTile[i].buffer);

            if (i == 0) {
                if (fromIsRAMBuffer) {
                    srcPtrs[0] = fromIsRAMBuffer->getData();
                    srcBounds = fromIsRAMBuffer->getBounds();
                    srcBitDepth = fromIsRAMBuffer->getBitDepth();
                    srcNComps = fromIsRAMBuffer->getNumComponents();

                    if (fromLayout == Image::eImageBufferLayoutRGBACoplanarFullRect) {
                        // Coplanar requires offsetting
                        assert(fromTile.perChannelTile.size() == 1);
                        std::size_t planeSize = srcNComps * srcBounds.area() * getSizeOfForBitDepth(srcBitDepth);
                        if (srcNComps > 1) {
                            srcPtrs[1] = (const char*)srcPtrs[0] + planeSize;
                            if (srcNComps > 2) {
                                srcPtrs[2] = (const char*)srcPtrs[1] + planeSize;
                                if (srcNComps > 3) {
                                    srcPtrs[3] = (const char*)srcPtrs[2] + planeSize;
                                }
                            }
                        }
                    }
                } else {
                    srcPtrs[0] = fromIsMMAPBuffer->getData();
                    srcBounds = fromIsMMAPBuffer->getBounds();
                    srcBitDepth = fromIsMMAPBuffer->getBitDepth();

                    // MMAP based storage only support mono channel images
                    srcNComps = 1;
                }
            } else {
                int channelIndex = fromTile.perChannelTile[i].channelIndex;
                assert(channelIndex >= 0 && channelIndex < 4);
                if (fromIsRAMBuffer) {
                    srcPtrs[channelIndex] = fromIsRAMBuffer->getData();
                } else {
                    srcPtrs[channelIndex] = fromIsMMAPBuffer->getData();
                }
            }
        }

        for (std::size_t i = 0; i < toTile.perChannelTile.size(); ++i) {
            RAMCacheEntryPtr toIsRAMBuffer = toRAMCacheEntry(toTile.perChannelTile[i].buffer);
            MemoryMappedCacheEntryPtr toIsMMAPBuffer = toMemoryMappedCacheEntry(toTile.perChannelTile[i].buffer);

            if (i == 0) {
                if (toIsRAMBuffer) {
                    dstPtrs[0] = toIsRAMBuffer->getData();
                    dstBounds = toIsRAMBuffer->getBounds();
                    dstBitDepth = toIsRAMBuffer->getBitDepth();
                    dstNComps = toIsRAMBuffer->getNumComponents();

                    if (toLayout == Image::eImageBufferLayoutRGBACoplanarFullRect) {
                        // Coplanar requires offsetting
                        assert(toTile.perChannelTile.size() == 1);
                        std::size_t planeSize = dstNComps * dstBounds.area() * getSizeOfForBitDepth(dstBitDepth);
                        if (dstNComps > 1) {
                            dstPtrs[1] = (char*)dstPtrs[0] + planeSize;
                            if (dstNComps > 2) {
                                dstPtrs[2] = (char*)dstPtrs[1] + planeSize;
                                if (dstNComps > 3) {
                                    dstPtrs[3] = (char*)dstPtrs[2] + planeSize;
                                }
                            }
                        }
                    }
                } else {
                    dstPtrs[0] = toIsMMAPBuffer->getData();
                    dstBounds = toIsMMAPBuffer->getBounds();
                    dstBitDepth = toIsMMAPBuffer->getBitDepth();

                    // MMAP based storage only support mono channel images
                    dstNComps = 1;
                }
            } else {
                int channelIndex = fromTile.perChannelTile[i].channelIndex;
                assert(channelIndex >= 0 && channelIndex < 4);
                if (toIsRAMBuffer) {
                    dstPtrs[channelIndex] = toIsRAMBuffer->getData();
                } else {
                    dstPtrs[channelIndex] = toIsMMAPBuffer->getData();
                }
            }
        }

        // This function is very optimized and templated for each cases.
        // In the best optimize case memcpy is used
        convertCPUImage(args.roi,
                        args.srcColorspace,
                        args.dstColorspace,
                        false /*unPremult*/,
                        args.conversionChannel,
                        args.alphaHandling,
                        args.monoConversion,
                        srcPtrs,
                        srcNComps,
                        srcBitDepth,
                        srcBounds,
                        dstPtrs,
                        dstNComps,
                        dstBitDepth,
                        dstBounds);

    } // storage cases
} // copyRectangle


NATRON_NAMESPACE_EXIT;
