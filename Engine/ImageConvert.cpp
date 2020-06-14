/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2020 The Natron developers
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
#include "Engine/Texture.h"
#include "Engine/Lut.h"

NATRON_NAMESPACE_ENTER

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
template <typename SRCPIX, int srcMaxValue, typename DSTPIX, int dstMaxValue>
ActionRetCodeEnum
convertToFormatInternal_sameComps(const RectI & renderWindow,
                                  ViewerColorSpaceEnum srcColorSpace,
                                  ViewerColorSpaceEnum dstColorSpace,
                                  const void* srcBufPtrs[4],
                                  int nComp,
                                  const RectI& srcBounds,
                                  void* dstBufPtrs[4],
                                  const RectI& dstBounds,
                                  const EffectInstancePtr& renderClone)
{

    const Color::Lut* const srcLut_ = lutFromColorspace(srcColorSpace);
    const Color::Lut* const dstLut_ = lutFromColorspace(dstColorSpace);


    // no colorspace conversion applied when luts are the same
    const Color::Lut* const srcLut = (srcLut_ == dstLut_) ? 0 : srcLut_;
    const Color::Lut* const dstLut = (srcLut_ == dstLut_) ? 0 : dstLut_;

    int srcDataSizeOf = sizeof(SRCPIX);


    for (int y = renderWindow.y1; y < renderWindow.y2; ++y) {

        if (renderClone && renderClone->isRenderAborted()) {
            return eActionStatusAborted;
        }

        if (srcMaxValue == dstMaxValue && !srcLut && !dstLut) {
            // Use memcpy when possible

            const SRCPIX* srcPixelPtrs[4] = {NULL, NULL, NULL, NULL};
            int srcPixelStride;
            Image::getChannelPointers<SRCPIX>((const SRCPIX**)srcBufPtrs, renderWindow.x1, y, srcBounds, nComp, (SRCPIX**)srcPixelPtrs, &srcPixelStride);

            DSTPIX* dstPixelPtrs[4] = {NULL, NULL, NULL, NULL};
            int dstPixelStride;
            Image::getChannelPointers<DSTPIX>((const DSTPIX**)dstBufPtrs, renderWindow.x1, y, dstBounds, nComp, (DSTPIX**)dstPixelPtrs, &dstPixelStride);


            // If the pixel stride is the same, use memcpy
            if (srcPixelStride == dstPixelStride) {
                if (srcPixelStride == 1) {
                    std::size_t nBytesToCopy = renderWindow.width() * srcDataSizeOf;
                    // Ok we are in coplanar mode, copy each channel individually
                    for (int c = 0; c < 4; ++c) {
                        if (srcPixelPtrs[c] && dstPixelPtrs[c]) {
                            memcpy(dstPixelPtrs[c], srcPixelPtrs[c], nBytesToCopy);
                        }
                    }
                } else {
                    std::size_t nBytesToCopy = renderWindow.width() * nComp * srcDataSizeOf;

                    // In packed RGBA mode or single channel coplanar a single call to memcpy is needed per scan-line
                    memcpy(dstPixelPtrs[0], srcPixelPtrs[0], nBytesToCopy);
                }
            } else {
                // Different strides, copy manually
                for (int c = 0; c < 4; ++c) {
                    if (srcPixelPtrs[c] && dstPixelPtrs[c]) {
                        const SRCPIX* src_pix = srcPixelPtrs[c];
                        DSTPIX* dst_pix = dstPixelPtrs[c];
                        for (int x = renderWindow.x1; x < renderWindow.x2; ++x) {
                            // They are of the same bitdepth since srcMaxValue == dstMaxValue was checked above
                            assert( !(boost::math::isnan)(*src_pix) ); // check for NaNs
                            *dst_pix = (DSTPIX)*src_pix;
                            src_pix += srcPixelStride;
                            dst_pix += dstPixelStride;
                        }
                    }
                }
            }

        } else {
            // Start of the line for error diffusion
            // coverity[dont_call]
            int start = rand() % renderWindow.width() + renderWindow.x1;

            const SRCPIX* srcPixelPtrs[4] = {NULL, NULL, NULL, NULL};
            int srcPixelStride;
            Image::getChannelPointers<SRCPIX>((const SRCPIX**)srcBufPtrs, start, y, srcBounds, nComp, (SRCPIX**)srcPixelPtrs, &srcPixelStride);

            DSTPIX* dstPixelPtrs[4] = {NULL, NULL, NULL, NULL};
            int dstPixelStride;
            Image::getChannelPointers<DSTPIX>((const DSTPIX**)dstBufPtrs, start, y, dstBounds, nComp, (DSTPIX**)dstPixelPtrs, &dstPixelStride);

            const SRCPIX* srcPixelStart[4];
            DSTPIX* dstPixelStart[4];
            memcpy(srcPixelStart, srcPixelPtrs, sizeof(SRCPIX*) * 4);
            memcpy(dstPixelStart, dstPixelPtrs, sizeof(DSTPIX*) * 4);
            

            for (int backward = 0; backward < 2; ++backward) {
                int x = backward ? std::max(renderWindow.x1 - 1, start - 1) : start;
                int end = backward ? renderWindow.x1 - 1 : renderWindow.x2;
                unsigned error[3] = {
                    0x80, 0x80, 0x80
                };

                while (x != end) {
                    for (int k = 0; k < nComp; ++k) {

                        if (!dstPixelPtrs[k]) {
                            continue;
                        }
                        SRCPIX sourcePixel;
                        if (srcPixelPtrs[k]) {
                            sourcePixel = *srcPixelPtrs[k];
                            assert( !(boost::math::isnan)(sourcePixel) ); // check for NaNs
                        } else {
                            sourcePixel = 0;
                        }

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
#                     ifdef DEBUG
                        assert( !(boost::math::isnan)(pix) ); // check for NaN
#                     endif
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
    return eActionStatusOK;
} // convertToFormatInternal_sameComps

template <typename SRCPIX, int srcMaxValue, typename DSTPIX, int dstMaxValue, int srcNComps, int dstNComps>
ActionRetCodeEnum
convertToFormatInternal(const RectI & renderWindow,
                               ViewerColorSpaceEnum srcColorSpace,
                               ViewerColorSpaceEnum dstColorSpace,
                               bool requiresUnpremult,
                               int conversionChannel,
                               Image::AlphaChannelHandlingEnum alphaHandling,
                               const void* srcBufPtrs[4],
                               const RectI& srcBounds,
                               void* dstBufPtrs[4],
                               const RectI& dstBounds,
                               const EffectInstancePtr& renderClone)
{
    // Other cases are optimizes in convertFromMono and convertToMono
    assert(dstNComps > 1 && srcNComps > 1);

    const Color::Lut* const srcLut = lutFromColorspace( (ViewerColorSpaceEnum)srcColorSpace );
    const Color::Lut* const dstLut = lutFromColorspace( (ViewerColorSpaceEnum)dstColorSpace );

    for (int y = renderWindow.y1; y < renderWindow.y2; ++y) {
        // Start of the line for error diffusion
        // coverity[dont_call]

        if (renderClone && renderClone->isRenderAborted()) {
            return eActionStatusAborted;
        }

        int start = rand() % renderWindow.width() + renderWindow.x1;

        const SRCPIX* srcPixelPtrs[4] = {NULL, NULL, NULL, NULL};
        int srcPixelStride;
        Image::getChannelPointers<SRCPIX, srcNComps>((const SRCPIX**)srcBufPtrs, start, y, srcBounds, (SRCPIX**)srcPixelPtrs, &srcPixelStride);

        DSTPIX* dstPixelPtrs[4] = {NULL, NULL, NULL, NULL};
        int dstPixelStride;
        Image::getChannelPointers<DSTPIX, dstNComps>((const DSTPIX**)dstBufPtrs, start, y, dstBounds, (DSTPIX**)dstPixelPtrs, &dstPixelStride);

        const SRCPIX* srcPixelStart[4];
        DSTPIX* dstPixelStart[4];
        memcpy(srcPixelStart, srcPixelPtrs, sizeof(SRCPIX*) * 4);
        memcpy(dstPixelStart, dstPixelPtrs, sizeof(DSTPIX*) * 4);


        // We do twice the loop, once from starting point to end and once from starting point - 1 to real start
        for (int backward = 0; backward < 2; ++backward) {

            int x = backward ? std::max(renderWindow.x1 - 1, start - 1) : start;

            // End is pointing to the first pixel outside the line a la stl
            int end = backward ? renderWindow.x1 - 1 : renderWindow.x2;

            // The error will be updated and diffused throughout the scanline
            unsigned error[3] = {0x80, 0x80, 0x80};

            while (x != end) {

                // We've XY, RGB or RGBA input and outputs
                assert(srcNComps != dstNComps);

                // This is only set if unpremultChannel is true
                float alphaForUnPremult;
                if (requiresUnpremult && srcPixelPtrs[3]) {
                    alphaForUnPremult = Image::convertPixelDepth<SRCPIX, float>(*srcPixelPtrs[3]);
                } else {
                    alphaForUnPremult = 1.;
                }

                // For RGB channels, unpremult and do colorspace conversion if needed.
                // For all channels, converting pixel depths is required at the very least.
                for (int k = 0; k < 3 && k < dstNComps; ++k) {
                    if (k >= srcNComps) { // e.g. srcNComps = 2 && dstNComps == 3 or 4
                        *dstPixelPtrs[k] =  0;
                        continue;
                    }

                    assert(dstPixelPtrs[k]);

                    SRCPIX sourcePixel = srcPixelPtrs[k] ? *srcPixelPtrs[k] : 0;

                    DSTPIX pix;
                    if (!srcLut && !dstLut) {
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

                        // Unpremult before doing colorspace conversion from linear to X
                        if (requiresUnpremult) {
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
                    } // use color spaces

                    *dstPixelPtrs[k] =  pix;
#                 ifdef DEBUG
                    assert( !(boost::math::isnan)(*dstPixelPtrs[k]) ); // check for NaN
#                 endif


                } // for (int k = 0; k < k < 3 && k < dstNComps; ++k) {

                if (dstPixelPtrs[3] && !srcPixelPtrs[3]) {
                    // we reach here only if converting RGB-->RGBA or XY--->RGBA
                    DSTPIX pix;
                    switch (alphaHandling) {
                        case Image::eAlphaChannelHandlingCreateFill0:
                        default:
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
    return eActionStatusOK;
} // convertToFormatInternal

template <typename SRCPIX, int srcMaxValue, typename DSTPIX, int dstMaxValue, int srcNComps, Image::AlphaChannelHandlingEnum alphaHandling>
ActionRetCodeEnum
convertToMonoImage(const RectI & renderWindow,
                          int conversionChannel,
                          const void* srcBufPtrs[4],
                          const RectI& srcBounds,
                          void* dstBufPtrs[4],
                          const RectI& dstBounds,
                          const EffectInstancePtr& renderClone)
{
    assert(dstBufPtrs[0] && !dstBufPtrs[1] && !dstBufPtrs[2] && !dstBufPtrs[3]);

    DSTPIX pix;

    for (int y = renderWindow.y1; y < renderWindow.y2; ++y) {
        // Start of the line for error diffusion
        // coverity[dont_call]


        if (renderClone && renderClone->isRenderAborted()) {
            return eActionStatusAborted;
        }

        const SRCPIX* srcPixelPtrs[4] = {NULL, NULL, NULL, NULL};
        int srcPixelStride;
        Image::getChannelPointers<SRCPIX, srcNComps>((const SRCPIX**)srcBufPtrs, renderWindow.x1, y, srcBounds, (SRCPIX**)srcPixelPtrs, &srcPixelStride);

        DSTPIX* dstPixelPtrs[4] = {NULL, NULL, NULL, NULL};
        int dstPixelStride;
        Image::getChannelPointers<DSTPIX, 1>((const DSTPIX**)dstBufPtrs, renderWindow.x1, y, dstBounds, (DSTPIX**)dstPixelPtrs, &dstPixelStride);


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
                    pix = Image::convertPixelDepth<SRCPIX, DSTPIX>(*srcPixelPtrs[conversionChannel]);
                    // Only increment the conversion channel pointer, this is the only one we use
                    srcPixelPtrs[conversionChannel] += srcPixelStride;
                    break;
                    
            }
            assert( !(boost::math::isnan)(pix) ); // Check for NaNs
            *dstPixelPtrs[0] = pix;
            dstPixelPtrs[0] += dstPixelStride;
        }
    }
    return eActionStatusOK;
} // convertToMonoImage

template <typename SRCPIX, int srcMaxValue, typename DSTPIX, int dstMaxValue, int dstNComps, Image::MonoToPackedConversionEnum monoConversion>
ActionRetCodeEnum
convertFromMonoImage(const RectI & renderWindow,
                            int conversionChannel,
                            const void* srcBufPtrs[4],
                            const RectI& srcBounds,
                            void* dstBufPtrs[4],
                            const RectI& dstBounds,
                            const EffectInstancePtr& renderClone)
{
    

    assert(srcBufPtrs[0] && !srcBufPtrs[1] && !srcBufPtrs[2] && !srcBufPtrs[3]);

    DSTPIX pix;

    for (int y = renderWindow.y1; y < renderWindow.y2; ++y) {
        // Start of the line for error diffusion
        // coverity[dont_call]

        if (renderClone && renderClone->isRenderAborted()) {
            return eActionStatusAborted;
        }

        const SRCPIX* srcPixelPtrs[4] = {NULL, NULL, NULL, NULL};
        int srcPixelStride;
        Image::getChannelPointers<SRCPIX, 1>((const SRCPIX**)srcBufPtrs, renderWindow.x1, y, srcBounds, (SRCPIX**)srcPixelPtrs, &srcPixelStride);

        DSTPIX* dstPixelPtrs[4] = {NULL, NULL, NULL, NULL};
        int dstPixelStride;
        Image::getChannelPointers<DSTPIX, dstNComps>((const DSTPIX**)dstBufPtrs, renderWindow.x1, y, dstBounds, (DSTPIX**)dstPixelPtrs, &dstPixelStride);

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
    return eActionStatusOK;
} // convertFromMonoImage

template <typename SRCPIX, int srcMaxValue, typename DSTPIX, int dstMaxValue, Image::MonoToPackedConversionEnum monoConversion>
ActionRetCodeEnum
convertFromMonoImageForComps(const RectI & renderWindow,
                                    int conversionChannel,
                                    const void* srcBufPtrs[4],
                                    const RectI& srcBounds,
                                    void* dstBufPtrs[4],
                                    int dstNComps,
                                    const RectI& dstBounds,
                                    const EffectInstancePtr& renderClone)
{
    switch (dstNComps) {
        case 2:
            return convertFromMonoImage<SRCPIX, srcMaxValue, DSTPIX, dstMaxValue, 2, monoConversion>(renderWindow, conversionChannel, srcBufPtrs, srcBounds, dstBufPtrs, dstBounds, renderClone);
        case 3:
            return convertFromMonoImage<SRCPIX, srcMaxValue, DSTPIX, dstMaxValue, 3, monoConversion>(renderWindow, conversionChannel, srcBufPtrs, srcBounds, dstBufPtrs, dstBounds, renderClone);
        case 4:
            return convertFromMonoImage<SRCPIX, srcMaxValue, DSTPIX, dstMaxValue, 4, monoConversion>(renderWindow, conversionChannel, srcBufPtrs, srcBounds, dstBufPtrs, dstBounds, renderClone);
        default:
            assert(false);
            break;
    }
    return eActionStatusFailed;

}


template <typename SRCPIX, int srcMaxValue, typename DSTPIX, int dstMaxValue, int srcNComps>
ActionRetCodeEnum
convertToFormatInternalForSrcComps(const RectI & renderWindow,
                                   ViewerColorSpaceEnum srcColorSpace,
                                   ViewerColorSpaceEnum dstColorSpace,
                                   bool requiresUnpremult,
                                   int conversionChannel,
                                   Image::AlphaChannelHandlingEnum alphaHandling,
                                   const void* srcBufPtrs[4],
                                   const RectI& srcBounds,
                                   void* dstBufPtrs[4],
                                   int dstNComps,
                                   const RectI& dstBounds,
                                   const EffectInstancePtr& renderClone)
{
    switch (dstNComps) {
        case 1:
        {
            // When converting to alpha, do not colorspace conversion
            // optimize the conversion
            switch (alphaHandling) {
                case Image::eAlphaChannelHandlingFillFromChannel:
                    return convertToMonoImage<SRCPIX, srcMaxValue, DSTPIX, dstMaxValue, srcNComps, Image::eAlphaChannelHandlingFillFromChannel>(renderWindow, conversionChannel, srcBufPtrs, srcBounds, dstBufPtrs, dstBounds, renderClone);
                case Image::eAlphaChannelHandlingCreateFill1:
                    return convertToMonoImage<SRCPIX, srcMaxValue, DSTPIX, dstMaxValue, srcNComps, Image::eAlphaChannelHandlingCreateFill1>(renderWindow, conversionChannel, srcBufPtrs, srcBounds, dstBufPtrs, dstBounds, renderClone);
                case Image::eAlphaChannelHandlingCreateFill0:
                    return convertToMonoImage<SRCPIX, srcMaxValue, DSTPIX, dstMaxValue, srcNComps, Image::eAlphaChannelHandlingCreateFill0>(renderWindow, conversionChannel, srcBufPtrs, srcBounds, dstBufPtrs, dstBounds, renderClone);
            }
        }   break;
        case 2:
            return convertToFormatInternal<SRCPIX, srcMaxValue, DSTPIX, dstMaxValue, srcNComps, 2>(renderWindow, srcColorSpace, dstColorSpace, requiresUnpremult, conversionChannel, alphaHandling, srcBufPtrs, srcBounds, dstBufPtrs, dstBounds, renderClone);
        case 3:
            return convertToFormatInternal<SRCPIX, srcMaxValue, DSTPIX, dstMaxValue, srcNComps, 3>(renderWindow, srcColorSpace, dstColorSpace, requiresUnpremult, conversionChannel, alphaHandling, srcBufPtrs, srcBounds, dstBufPtrs, dstBounds, renderClone);
        case 4:
            return convertToFormatInternal<SRCPIX, srcMaxValue, DSTPIX, dstMaxValue, srcNComps, 4>(renderWindow, srcColorSpace, dstColorSpace, requiresUnpremult, conversionChannel, alphaHandling, srcBufPtrs, srcBounds, dstBufPtrs, dstBounds, renderClone);
        default:
            assert(false);
            break;
    }
    return eActionStatusFailed;
}

template <typename SRCPIX, int srcMaxValue, typename DSTPIX, int dstMaxValue>
ActionRetCodeEnum
convertToFormatInternalForDstDepth(const RectI & renderWindow,
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
                                   const RectI& dstBounds,
                                   const EffectInstancePtr& renderClone)
{

    if (srcNComps == dstNComps) {
        // Optimize same number of components
        return convertToFormatInternal_sameComps<SRCPIX, srcMaxValue, DSTPIX, dstMaxValue>(renderWindow, srcColorSpace, dstColorSpace, srcBufPtrs, srcNComps, srcBounds, dstBufPtrs, dstBounds, renderClone);
    }

    if (srcNComps == 1) {
        // Mono image to something else, optimize
        switch (monoConversion) {
            case Image::eMonoToPackedConversionCopyToChannelAndLeaveOthers:
                return convertFromMonoImageForComps<SRCPIX, srcMaxValue, DSTPIX, dstMaxValue, Image::eMonoToPackedConversionCopyToChannelAndLeaveOthers>(renderWindow, conversionChannel, srcBufPtrs, srcBounds, dstBufPtrs, dstNComps, dstBounds, renderClone);
            case Image::eMonoToPackedConversionCopyToChannelAndFillOthers:
                return convertFromMonoImageForComps<SRCPIX, srcMaxValue, DSTPIX, dstMaxValue, Image::eMonoToPackedConversionCopyToChannelAndFillOthers>(renderWindow, conversionChannel, srcBufPtrs, srcBounds, dstBufPtrs, dstNComps, dstBounds, renderClone);
            case Image::eMonoToPackedConversionCopyToAll:
                return convertFromMonoImageForComps<SRCPIX, srcMaxValue, DSTPIX, dstMaxValue, Image::eMonoToPackedConversionCopyToAll>(renderWindow, conversionChannel, srcBufPtrs, srcBounds, dstBufPtrs, dstNComps, dstBounds, renderClone);
        }
    }
    switch (srcNComps) {

        case 2:
            return convertToFormatInternalForSrcComps<SRCPIX, srcMaxValue, DSTPIX, dstMaxValue, 2>(renderWindow, srcColorSpace, dstColorSpace, requiresUnpremult, conversionChannel, alphaHandling, srcBufPtrs, srcBounds, dstBufPtrs, dstNComps, dstBounds, renderClone);
        case 3:
            return convertToFormatInternalForSrcComps<SRCPIX, srcMaxValue, DSTPIX, dstMaxValue, 3>(renderWindow, srcColorSpace, dstColorSpace, requiresUnpremult, conversionChannel, alphaHandling, srcBufPtrs, srcBounds, dstBufPtrs, dstNComps, dstBounds, renderClone);
        case 4:
            return convertToFormatInternalForSrcComps<SRCPIX, srcMaxValue, DSTPIX, dstMaxValue, 4>(renderWindow, srcColorSpace, dstColorSpace, requiresUnpremult, conversionChannel, alphaHandling, srcBufPtrs, srcBounds, dstBufPtrs, dstNComps, dstBounds, renderClone);
        default:
            break;
    }
    return eActionStatusFailed;
} // convertToFormatInternalForDstDepth

template <typename SRCPIX, int srcMaxValue>
ActionRetCodeEnum
convertToFormatInternalForSrcDepth(const RectI & renderWindow,
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
                                   ImageBitDepthEnum dstBitDepth,
                                   const RectI& dstBounds,
                                   const EffectInstancePtr& renderClone)
{
    switch ( dstBitDepth ) {
        case eImageBitDepthByte:
            ///Same as a copy
            return convertToFormatInternalForDstDepth<SRCPIX, srcMaxValue, unsigned char, 255>(renderWindow, srcColorSpace, dstColorSpace, requiresUnpremult, conversionChannel, alphaHandling, monoConversion, srcBufPtrs, srcNComps, srcBounds, dstBufPtrs, dstNComps, dstBounds, renderClone);
        case eImageBitDepthShort:
            return convertToFormatInternalForDstDepth<SRCPIX, srcMaxValue, unsigned short, 65535>(renderWindow, srcColorSpace, dstColorSpace, requiresUnpremult, conversionChannel, alphaHandling, monoConversion, srcBufPtrs, srcNComps, srcBounds, dstBufPtrs, dstNComps, dstBounds, renderClone);
        case eImageBitDepthHalf:
            break;
        case eImageBitDepthFloat:
            return convertToFormatInternalForDstDepth<SRCPIX, srcMaxValue, float, 1>(renderWindow, srcColorSpace, dstColorSpace, requiresUnpremult, conversionChannel, alphaHandling, monoConversion, srcBufPtrs, srcNComps, srcBounds, dstBufPtrs, dstNComps, dstBounds, renderClone);
        case eImageBitDepthNone:
            break;
    }
    return eActionStatusFailed;
}

ActionRetCodeEnum
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
                              const RectI& dstBounds,
                              const EffectInstancePtr& renderClone)
{
    assert( srcBounds.contains(renderWindow) && dstBounds.contains(renderWindow) );

    switch ( srcBitDepth ) {
        case eImageBitDepthByte:
            ///Same as a copy
            return convertToFormatInternalForSrcDepth<unsigned char, 255>(renderWindow, srcColorSpace, dstColorSpace, requiresUnpremult, conversionChannel, alphaHandling, monoConversion, srcBufPtrs, srcNComps, srcBounds, dstBufPtrs, dstNComps, dstBitDepth, dstBounds, renderClone);
        case eImageBitDepthShort:
            return convertToFormatInternalForSrcDepth<unsigned short, 65535>(renderWindow, srcColorSpace, dstColorSpace, requiresUnpremult, conversionChannel, alphaHandling, monoConversion, srcBufPtrs, srcNComps, srcBounds, dstBufPtrs, dstNComps, dstBitDepth, dstBounds, renderClone);
        case eImageBitDepthHalf:
            break;
        case eImageBitDepthFloat:
            return convertToFormatInternalForSrcDepth<float, 1>(renderWindow, srcColorSpace, dstColorSpace, requiresUnpremult, conversionChannel, alphaHandling, monoConversion, srcBufPtrs, srcNComps, srcBounds, dstBufPtrs, dstNComps, dstBitDepth, dstBounds, renderClone);
        case eImageBitDepthNone:
            break;
    }
    return eActionStatusFailed;
} // convertCPUImage

template <typename GL>
void
copyGLTextureInternal(const GLTexturePtr& from,
                      const GLTexturePtr& to,
                      const RectI& roi,
                      const OSGLContextPtr& glContext)
{
    // The OpenGL context must be current to this thread.
    assert(appPTR->getGPUContextPool()->getThreadLocalContext()->getContext() == glContext);



    // Texture targets must be the same
    int target = from->getTexTarget();
    assert(target == to->getTexTarget());

    GLuint fboID = glContext->getOrCreateFBOId();
    GL::Disable(GL_SCISSOR_TEST);
    GL::BindFramebuffer(GL_FRAMEBUFFER, fboID);
    GL::Enable(target);
    GL::ActiveTexture(GL_TEXTURE0);

    U32 toTexID = to->getTexID();
    U32 fromTexID = from->getTexID();

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

    OSGLContext::applyTextureMapping<GL>(from->getBounds(), to->getBounds(), roi);

    shader->unbind();
    GL::BindTexture(target, 0);

    glCheckError(GL);
} // copyGLTextureInternal

void
ImagePrivate::copyGLTexture(const GLTexturePtr& from,
                            const GLTexturePtr& to,
                            const RectI& roi,
                            const OSGLContextPtr& glContext)
{
    if (glContext->isGPUContext()) {
        copyGLTextureInternal<GL_GPU>(from, to, roi, glContext);
    } else {
        copyGLTextureInternal<GL_CPU>(from, to, roi, glContext);
    }
}

static void
copyGLTextureWrapper(const GLImageStoragePtr& from,
              const GLImageStoragePtr& to,
              const RectI& roi,
              const OSGLContextPtr& glContext)
{

    // Textures must belong to the same context
    assert(glContext == from->getOpenGLContext() &&
           glContext == to->getOpenGLContext());

    ImagePrivate::copyGLTexture(from->getTexture(), to->getTexture(), roi, glContext);

}

template <typename GL>
ActionRetCodeEnum
convertRGBAPackedCPUBufferToGLTextureInternal(const float* srcData,
                                              const RectI& srcBounds,
                                              int srcNComps,
                                              const RectI& roi,
                                              const GLTexturePtr& outTexture,
                                              const OSGLContextPtr& glContext)
{

    // The OpenGL context must be current to this thread.
    assert(appPTR->getGPUContextPool()->getThreadLocalContext()->getContext() == glContext);

    // Only RGBA 32-bit textures for now
    assert(srcNComps == 4);
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
    glCheckError(GL);
    if (!gpuData) {
        return eActionStatusFailed;
    }
    const RectI toBounds = outTexture->getBounds();
    assert(toBounds.contains(roi));
    // Copy the CPU buffer to the PBO
    {
        const std::size_t srcRowBytes = srcBounds.width() * srcNComps * sizeof(float);
        {
            unsigned char* dstData = gpuData;
            const unsigned char* srcPixels = Image::pixelAtStatic(roi.x1, roi.y1, srcBounds, srcNComps, sizeof(float), (const unsigned char*)srcData);
            for (int y = roi.y1; y < roi.y2; ++y) {
#ifdef DEBUG
                float* dstLineData = (float*)dstData;
                const float* srcLineData = (const float*)srcPixels;
                for (int x = roi.x1; x < roi.x2; ++x, dstLineData += 4, srcLineData += srcNComps) {
                    for (int c = 0; c < 4; ++c) {
                        assert( !(boost::math::isnan)(srcLineData[c]) ); // Check for NaNs
                        dstLineData[c] = srcLineData[c];
                    }
                }
#else
                memcpy(dstData, srcPixels, pboRowBytes);
#endif
                srcPixels += srcRowBytes;
                dstData += pboRowBytes;

            }
        }


        GLboolean result = GL::UnmapBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB); // release the mapped buffer
        assert(result == GL_TRUE);
        Q_UNUSED(result);
        
        glCheckError(GL);
    }

    U32 target = outTexture->getTexTarget();
    U32 outTextureID = outTexture->getTexID();

    // bind the texture
    GL::BindTexture( target, outTextureID );
    glCheckError(GL);
    // copy pixels from PBO to texture object
    // Use offset instead of pointer (last parameter is 0).
    GL::TexSubImage2D(target,
                      0,              // level
                      roi.x1 - toBounds.x1, roi.y1 - toBounds.y1,               // xoffset, yoffset
                      roi.width(), roi.height(),
                      outTexture->getFormat(),            // format
                      outTexture->getGLType(),       // type
                      0);
    glCheckError(GL);
    GL::BindTexture(target, 0);


    // it is good idea to release PBOs with ID 0 after use.
    // Once bound with 0, all pixel operations are back to normal ways.
    GL::BindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, 0);
    glCheckError(GL);
    return eActionStatusOK;
}  // convertRGBAPackedCPUBufferToGLTextureInternal

ActionRetCodeEnum
ImagePrivate::convertRGBAPackedCPUBufferToGLTexture(const float* srcData,
                                      const RectI& srcBounds,
                                      int srcNComps,
                                      const RectI& roi,
                                      const GLTexturePtr& outTexture,
                                      const OSGLContextPtr& glContext)
{
    if (glContext->isGPUContext()) {
        return convertRGBAPackedCPUBufferToGLTextureInternal<GL_GPU>(srcData, srcBounds, srcNComps, roi, outTexture, glContext);
    } else {
        return convertRGBAPackedCPUBufferToGLTextureInternal<GL_CPU>(srcData, srcBounds, srcNComps, roi, outTexture, glContext);
    }
}

static ActionRetCodeEnum
convertRGBAPackedCPUBufferToGLTextureWrapper(const RAMImageStoragePtr& buffer, const RectI& roi, const GLImageStoragePtr& outTexture)
{
    OSGLContextPtr glContext = outTexture->getOpenGLContext();
    assert(glContext == outTexture->getOpenGLContext());
    // This function only supports RGBA float input buffer
    assert(buffer->getNumComponents() == 4 && buffer->getBitDepth() == eImageBitDepthFloat);
    return ImagePrivate::convertRGBAPackedCPUBufferToGLTexture((const float*)buffer->getData(), buffer->getBounds(), 4, roi, outTexture->getTexture(), glContext);

}

template <typename GL>
static ActionRetCodeEnum
convertGLTextureToRGBAPackedCPUBufferInternal(const GLTexturePtr& texture,
                                              const RectI& roi,
                                              float* outBuffer,
                                              const RectI& dstBounds,
                                              int dstNComps,
                                              const OSGLContextPtr& glContext)
{
    // The OpenGL context must be current to this thread.
    assert(appPTR->getGPUContextPool()->getThreadLocalContext()->getContext() == glContext);


    // glReadPixels expects the CPU buffer to have the appropriate size: we don't have any means to specify a stride.
    assert(dstBounds == roi);
    assert(dstNComps == 4);

    GLuint fboID = glContext->getOrCreateFBOId();

    int target = texture->getTexTarget();
    U32 texID = texture->getTexID();
    RectI texBounds = texture->getBounds();

    // The texture must be read in a buffer with the same bounds
    assert(dstBounds == texBounds);


    GLint currentFBO, currentTex;
    GL::GetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &currentFBO);
    GL::GetIntegerv(GL_TEXTURE_BINDING_2D, &currentTex);

    GL::BindFramebuffer(GL_FRAMEBUFFER, fboID);
    GL::Enable(target);
    GL::BindTexture( target, texID );
    GL::FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, target, texID, 0 /*LoD*/);

    GL::Viewport( texBounds.x1, texBounds.y1, texBounds.width(), texBounds.height() );
    glCheckFramebufferError(GL);

    // Ensure all drawing commands are finished
    // Transfer the texture to the CPU buffer. Note that we do not use a PBO here because we cannot let OpenGL do asynchronous drawing commands: we
    // need the data now.
    GL::Flush();
    GL::Finish();
    glCheckError(GL);

    {
        unsigned char* data = Image::pixelAtStatic(roi.x1, roi.y1, dstBounds, dstNComps, sizeof(float), (unsigned char*)outBuffer);
        GL::ReadPixels(roi.x1 - texBounds.x1, roi.y1 - texBounds.y1, roi.width(), roi.height(), texture->getFormat(), texture->getGLType(), (GLvoid*)data);
    }

    GL::BindTexture(target, currentTex);
    GL::BindFramebuffer(GL_FRAMEBUFFER, currentFBO);
    glCheckError(GL);
    return eActionStatusOK;

} // convertGLTextureToRGBAPackedCPUBufferInternal

ActionRetCodeEnum
ImagePrivate::convertGLTextureToRGBAPackedCPUBuffer(const GLTexturePtr& texture,
                                                    const RectI& roi,
                                                    float* outBuffer,
                                                    const RectI& dstBounds,
                                                    int dstNComps,
                                                    const OSGLContextPtr& glContext)
{
    if (glContext->isGPUContext()) {
        return convertGLTextureToRGBAPackedCPUBufferInternal<GL_GPU>(texture, roi, outBuffer, dstBounds, dstNComps, glContext);
    } else {
        return convertGLTextureToRGBAPackedCPUBufferInternal<GL_CPU>(texture, roi, outBuffer, dstBounds, dstNComps, glContext);
    }

}

static ActionRetCodeEnum
convertGLTextureToRGBAPackedCPUBufferWrapper(const GLImageStoragePtr& texture,
                                      const RectI& roi,
                                      const RAMImageStoragePtr& outBuffer,
                                      const OSGLContextPtr& glContext)
{
    assert(glContext == texture->getOpenGLContext());

    // This function only supports reading to a RGBA float buffer.
    assert(outBuffer->getNumComponents() == 4 && outBuffer->getBitDepth() == eImageBitDepthFloat);
    return ImagePrivate::convertGLTextureToRGBAPackedCPUBuffer(texture->getTexture(), roi, (float*)outBuffer->getData(), outBuffer->getBounds(), outBuffer->getNumComponents(), glContext);
}


class CopyPixelsProcessor : public ImageMultiThreadProcessorBase
{
    Image::CPUData _srcTileData, _dstTileData;
    Image::CopyPixelsArgs _copyArgs;
public:

    CopyPixelsProcessor(const EffectInstancePtr& renderClone)
    : ImageMultiThreadProcessorBase(renderClone)
    , _srcTileData()
    , _dstTileData()
    , _copyArgs()
    {

    }

    virtual ~CopyPixelsProcessor()
    {
    }

    void setValues(const Image::CPUData& srcTileData,
                   const Image::CPUData& dstTileData,
                   const Image::CopyPixelsArgs& copyArgs)
    {
        _srcTileData = srcTileData;
        _dstTileData = dstTileData;
        _copyArgs = copyArgs;
    }

private:

    virtual ActionRetCodeEnum multiThreadProcessImages(const RectI& renderWindow) OVERRIDE FINAL
    {
        // This function is very optimized and templated for most common cases
        // In the best optimized case, memcpy is used
        return ImagePrivate::convertCPUImage(renderWindow,
                                      _copyArgs.srcColorspace,
                                      _copyArgs.dstColorspace,
                                      _copyArgs.unPremultIfNeeded,
                                      _copyArgs.conversionChannel,
                                      _copyArgs.alphaHandling,
                                      _copyArgs.monoConversion,
                                      (const void**)_srcTileData.ptrs,
                                      _srcTileData.nComps,
                                      _srcTileData.bitDepth,
                                      _srcTileData.bounds,
                                      (void**)_dstTileData.ptrs,
                                      _dstTileData.nComps,
                                      _dstTileData.bitDepth,
                                      _dstTileData.bounds,
                                      _effect);

    }
};


ActionRetCodeEnum
ImagePrivate::copyPixelsInternal(const ImagePrivate* fromImage,
                                 ImagePrivate* toImage,
                                 const Image::CopyPixelsArgs& args,
                                 const EffectInstancePtr& renderClone)
{


    if (fromImage->storage == eStorageModeGLTex && toImage->storage == eStorageModeGLTex) {
        

        // GL texture to GL texture
        assert(fromImage->bufferFormat == eImageBufferLayoutRGBAPackedFullRect &&
               toImage->bufferFormat == eImageBufferLayoutRGBAPackedFullRect);

        GLImageStoragePtr fromIsGLTexture = toGLImageStorage(fromImage->channels[0]);
        GLImageStoragePtr toIsGLTexture = toGLImageStorage(toImage->channels[0]);

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
            copyGLTextureWrapper(fromIsGLTexture, toIsGLTexture, args.roi, fromIsGLTexture->getOpenGLContext());
        }
        return eActionStatusOK;

    } else if (fromImage->storage == eStorageModeGLTex && toImage->storage != eStorageModeGLTex) {

        // GL texture to CPU
        assert(fromImage->bufferFormat == eImageBufferLayoutRGBAPackedFullRect &&
               toImage->bufferFormat == eImageBufferLayoutRGBAPackedFullRect);


        GLImageStoragePtr fromIsGLTexture = toGLImageStorage(fromImage->channels[0]);
        RAMImageStoragePtr toIsRAMBuffer = toRAMImageStorage(toImage->channels[0]);

        // The buffer can only be a RAM buffer because MMAP only supports mono channel tiles
        // which is not supported for the conversion to OpenGL textures.
        assert(fromIsGLTexture && toIsRAMBuffer);
        assert(toImage->bufferFormat == eImageBufferLayoutRGBAPackedFullRect && toIsRAMBuffer->getNumComponents() == 4);

        // Save the current context
        OSGLContextSaver saveCurrentContext;
        ActionRetCodeEnum stat;
        {
            // Ensure this context is attached
            OSGLContextAttacherPtr contextAttacher = OSGLContextAttacher::create(fromIsGLTexture->getOpenGLContext());
            contextAttacher->attach();
            stat = convertGLTextureToRGBAPackedCPUBufferWrapper(fromIsGLTexture, args.roi, toIsRAMBuffer, contextAttacher->getContext());
        }
        return stat;

    } else if (fromImage->storage != eStorageModeGLTex && toImage->storage == eStorageModeGLTex) {

        // CPU to GL texture
        assert(fromImage->bufferFormat == eImageBufferLayoutRGBAPackedFullRect &&
               toImage->bufferFormat == eImageBufferLayoutRGBAPackedFullRect);

        GLImageStoragePtr toIsGLTexture = toGLImageStorage(toImage->channels[0]);
        RAMImageStoragePtr fromIsRAMBuffer = toRAMImageStorage(fromImage->channels[0]);

        // The buffer can only be a RAM buffer because MMAP only supports mono channel tiles
        // which is not supported for the conversion to OpenGL textures.
        assert(toIsGLTexture && fromIsRAMBuffer);
        assert(fromImage->bufferFormat == eImageBufferLayoutRGBAPackedFullRect && fromIsRAMBuffer->getNumComponents() == 4);

        // Save the current context
        OSGLContextSaver saveCurrentContext;

        ActionRetCodeEnum stat;
        {
            // Ensure this context is attached
            OSGLContextAttacherPtr contextAttacher = OSGLContextAttacher::create(toIsGLTexture->getOpenGLContext());
            contextAttacher->attach();

            stat = convertRGBAPackedCPUBufferToGLTextureWrapper(fromIsRAMBuffer, args.roi, toIsGLTexture);
        }
        return stat;
    } else {

        // CPU to CPU

        // The pointer to the RGBA channels.
        Image::CPUData srcData, dstData;
        fromImage->_publicInterface->getCPUData(&srcData);
        toImage->_publicInterface->getCPUData(&dstData);

        CopyPixelsProcessor processor(renderClone);
        processor.setRenderWindow(args.roi);
        processor.setValues(srcData, dstData, args);
        return processor.process();

    } // storage cases
} // copyRectangle


NATRON_NAMESPACE_EXIT
