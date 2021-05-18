/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2021 The Natron developers
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

#include "Image.h"

#include <algorithm> // min, max
#include <cassert>
#include <stdexcept>

#ifndef Q_MOC_RUN
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_OFF
#include <boost/math/special_functions/fpclassify.hpp>
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_ON
#endif

#include <QtCore/QDebug>

#include "Engine/AppManager.h"
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
template <typename SRCPIX, typename DSTPIX, int srcMaxValue, int dstMaxValue>
void
Image::convertToFormatInternal_sameComps(const RectI & renderWindow,
                                         const Image & srcImg,
                                         Image & dstImg,
                                         ViewerColorSpaceEnum srcColorSpace,
                                         ViewerColorSpaceEnum dstColorSpace,
                                         bool copyBitmap)
{
    const RectI & r = srcImg._bounds;
    RectI intersection;

    if ( !renderWindow.intersect(r, &intersection) ) {
        return;
    }

    ImageBitDepthEnum dstDepth = dstImg.getBitDepth();
    ImageBitDepthEnum srcDepth = srcImg.getBitDepth();
    int nComp = (int)srcImg.getComponentsCount();
    const Color::Lut* const srcLut_ = lutFromColorspace(srcColorSpace);
    const Color::Lut* const dstLut_ = lutFromColorspace(dstColorSpace);


    ///no colorspace conversion applied when luts are the same
    const Color::Lut* const srcLut = (srcLut_ == dstLut_) ? 0 : srcLut_;
    const Color::Lut* const dstLut = (srcLut_ == dstLut_) ? 0 : dstLut_;
    if ( intersection.isNull() ) {
        return;
    }
    for (int y = 0; y < intersection.height(); ++y) {
        // coverity[dont_call]
        int start = rand() % intersection.width();
        const SRCPIX* srcPixels = (const SRCPIX*)srcImg.pixelAt(intersection.x1 + start, intersection.y1 + y);
        DSTPIX* dstPixels = (DSTPIX*)dstImg.pixelAt(intersection.x1 + start, intersection.y1 + y);
        const SRCPIX* srcStart = srcPixels;
        DSTPIX* dstStart = dstPixels;

        for (int backward = 0; backward < 2; ++backward) {
            int x = backward ? start - 1 : start;
            int end = backward ? -1 : intersection.width();
            unsigned error[3] = {
                0x80, 0x80, 0x80
            };

            while ( x != end && x >= 0 && x < intersection.width() ) {
                for (int k = 0; k < nComp; ++k) {
#                 ifdef DEBUG
                    assert( !(boost::math::isnan)(srcPixels[k]) ); // check for NaN
#                 endif
                    DSTPIX pix;
                    if ( (k == 3) || (!srcLut && !dstLut) ) {
                        pix = convertPixelDepth<SRCPIX, DSTPIX>(srcPixels[k]);
                    } else {
                        float pixFloat;

                        if (srcLut) {
                            if (srcDepth == eImageBitDepthByte) {
                                pixFloat = srcLut->fromColorSpaceUint8ToLinearFloatFast(srcPixels[k]);
                            } else if (srcDepth == eImageBitDepthShort) {
                                pixFloat = srcLut->fromColorSpaceUint16ToLinearFloatFast(srcPixels[k]);
                            } else {
                                pixFloat = srcLut->fromColorSpaceFloatToLinearFloat(srcPixels[k]);
                            }
                        } else {
                            pixFloat = convertPixelDepth<SRCPIX, float>(srcPixels[k]);
                        }

                        if (dstDepth == eImageBitDepthByte) {
                            ///small increase in perf we use Luts. This should be anyway the most used case.
                            error[k] = (error[k] & 0xff) + ( dstLut ? dstLut->toColorSpaceUint8xxFromLinearFloatFast(pixFloat) :
                                                             Color::floatToInt<0xff01>(pixFloat) );
                            pix = error[k] >> 8;
                        } else if (dstDepth == eImageBitDepthShort) {
                            pix = dstLut ? dstLut->toColorSpaceUint16FromLinearFloatFast(pixFloat) :
                                  convertPixelDepth<float, DSTPIX>(pixFloat);
                        } else {
                            if (dstLut) {
                                pixFloat = dstLut->toColorSpaceFloatFromLinearFloat(pixFloat);
                            }
                            pix = convertPixelDepth<float, DSTPIX>(pixFloat);
                        }
                    }
                    dstPixels[k] =  pix;
#                 ifdef DEBUG
                    assert( !(boost::math::isnan)(dstPixels[k]) ); // check for NaN
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
        }

        if (copyBitmap) {
            dstImg.copyBitmapRowPortion(intersection.x1, intersection.x2, intersection.y1 + y, srcImg);
        }
    }
} // convertToFormatInternal_sameComps

template <typename SRCPIX, typename DSTPIX, int srcMaxValue, int dstMaxValue, int srcNComps, int dstNComps,
          bool requiresUnpremult, bool useColorspaces>
void
Image::convertToFormatInternalForColorSpace(const RectI & renderWindow,
                                            const Image & srcImg,
                                            Image & dstImg,
                                            bool copyBitmap,
                                            bool useAlpha0,
                                            ViewerColorSpaceEnum srcColorSpace,
                                            ViewerColorSpaceEnum dstColorSpace,
                                            int channelForAlpha)
{
    /*
     * If channelForAlpha is -1 the user wants to convert using the default
     * If channelForAlpha >= 0 then the user wants a specific channel to convert from. This is used mainly when converting
     * to Alpha images (masks) to know which channel to use for the mask.
     */
    if (channelForAlpha != -1) {
        switch (srcNComps) {
        case 3:
            //invalid value passed by the called
            if (channelForAlpha > 2) {
                channelForAlpha = -1;
            }
            break;
        case 2:
            //invalid value passed by the called
            if (channelForAlpha > 1) {
                channelForAlpha = -1;
            }
        default:
            break;
        }
    } else {
        switch (srcNComps) {
        case 4:
            channelForAlpha = 3;
            break;
        case 3:
        case 1:
        default:
            //no alpha anyway
            break;
        }
    }


    ///special case comp == alpha && channelForAlpha = -1 clear out the mask
    if ( (dstNComps == 1) && (channelForAlpha == -1) ) {
        DSTPIX* dstPixels = (DSTPIX*)dstImg.pixelAt(renderWindow.x1, renderWindow.y1);
        int dstRowSize = dstImg._bounds.width() * dstNComps;
        if (copyBitmap) {
            dstImg.copyBitmapPortion(renderWindow, srcImg);
        }
        for (int y = 0; y < renderWindow.height();
             ++y, dstPixels += dstRowSize) {
            std::fill(dstPixels, dstPixels + renderWindow.width() * dstNComps, 0.);
        }

        return;
    }

    const Color::Lut* const srcLut = useColorspaces ? lutFromColorspace( (ViewerColorSpaceEnum)srcColorSpace ) : 0;
    const Color::Lut* const dstLut = useColorspaces ? lutFromColorspace( (ViewerColorSpaceEnum)dstColorSpace ) : 0;

    for (int y = 0; y < renderWindow.height(); ++y) {
        ///Start of the line for error diffusion
        // coverity[dont_call]
        int start = rand() % renderWindow.width();
        const SRCPIX* srcPixels = (const SRCPIX*)srcImg.pixelAt(renderWindow.x1 + start, renderWindow.y1 + y);
        DSTPIX* dstPixels = (DSTPIX*)dstImg.pixelAt(renderWindow.x1 + start, renderWindow.y1 + y);
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
                if (dstNComps == 1) {
                    ///If we're converting to alpha, we just have to handle pixel depth conversion
                    DSTPIX pix;

                    // convertPixelDepth is optimized when SRCPIX == DSTPIX

                    switch (srcNComps) {
                    case 4:
                        //channel for alpha must be valid
                        assert(channelForAlpha > -1 && channelForAlpha <= 3);
                        pix = convertPixelDepth<SRCPIX, DSTPIX>(srcPixels[channelForAlpha]);
                        break;
                    case 3:
                        // RGB is opaque, so no alpha, unless channelForAlpha is 0-2
                        pix = convertPixelDepth<SRCPIX, DSTPIX>(channelForAlpha == -1 ? 0. : srcPixels[channelForAlpha]);
                        break;
                    case 2:
                        // XY is opaque unless channelForAlpha is  0-1
                        pix = convertPixelDepth<SRCPIX, DSTPIX>(channelForAlpha == -1 ? 0. : srcPixels[channelForAlpha]);
                        break;
                    case 1:
                        // just copy alpha disregarding channelForAlpha
                        pix  = convertPixelDepth<SRCPIX, DSTPIX>(*srcPixels);
                        break;
                    }

                    dstPixels[0] = pix;
#                 ifdef DEBUG
                    assert( !(boost::math::isnan)(dstPixels[0]) ); // check for NaN
#                 endif
                } else { // if (dstNComps == 1) {
                    if (srcNComps == 1) {
                        DSTPIX pix = convertPixelDepth<SRCPIX, DSTPIX>(srcPixels[0]);
#                     ifdef DEBUG
                        assert(  !(boost::math::isnan)(pix) ); // check for NaN
#                     endif
                        for (int k = 0; k < dstNComps; ++k) {
                            dstPixels[k] = pix;
                        }
                    } else {
                        ///In this case we've XY, RGB or RGBA input and outputs
                        assert(srcNComps != dstNComps);

                        const bool unpremultChannel = ( //srcNComps == 4 && // test already done in convertToFormatInternalForDepth
                                                        //dstNComps == 3 && // test already done in convertToFormatInternalForDepth
                            requiresUnpremult);

                        ///This is only set if unpremultChannel is true
                        float alphaForUnPremult;
                        if (unpremultChannel) {
                            alphaForUnPremult = convertPixelDepth<SRCPIX, float>(srcPixels[srcNComps - 1]);
                        } else {
                            alphaForUnPremult = 1.;
                        }

                        for (int k = 0; k < 3 && k < dstNComps; ++k) {
                            if (k >= srcNComps) { // e.g. srcNComps = 2 && dstNComps == 3 or 4
                                dstPixels[k] =  0;
                                continue;
                            }
                            SRCPIX sourcePixel = srcPixels[k];
                            DSTPIX pix;
                            if ( !useColorspaces || (!srcLut && !dstLut) ) {
                                if (dstMaxValue == 255) {
                                    float pixFloat = convertPixelDepth<SRCPIX, float>(sourcePixel);
                                    error[k] = (error[k] & 0xff) + Color::floatToInt<0xff01>(pixFloat);
                                    pix = error[k] >> 8;
                                } else {
                                    pix = convertPixelDepth<SRCPIX, DSTPIX>(sourcePixel);
                                }
                            } else {
                                ///For RGB channels
                                float pixFloat;

                                ///Unpremult before doing colorspace conversion from linear to X
                                if (unpremultChannel) {
                                    pixFloat = convertPixelDepth<SRCPIX, float>(sourcePixel);
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
                                    pixFloat = convertPixelDepth<SRCPIX, float>(sourcePixel);
                                }

                                ///Apply dst color-space
                                if (dstMaxValue == 255) {
                                    assert(k < 3);
                                    error[k] = (error[k] & 0xff) + ( dstLut ? dstLut->toColorSpaceUint8xxFromLinearFloatFast(pixFloat) :
                                                                     Color::floatToInt<0xff01>(pixFloat) );
                                    pix = error[k] >> 8;
                                } else if (dstMaxValue == 65535) {
                                    pix = dstLut ? dstLut->toColorSpaceUint16FromLinearFloatFast(pixFloat) :
                                          convertPixelDepth<float, DSTPIX>(pixFloat);
                                } else {
                                    if (dstLut) {
                                        pixFloat = dstLut->toColorSpaceFloatFromLinearFloat(pixFloat);
                                    }
                                    pix = convertPixelDepth<float, DSTPIX>(pixFloat);
                                }
                            } // if (!useColorspaces || (!srcLut && !dstLut)) {
                            dstPixels[k] =  pix;
#                 ifdef DEBUG
                            assert( (boost::math::isnan)(srcPixels[k]) || !(boost::math::isnan)(dstPixels[k]) ); // check for NaN
#                 endif
                        } // for (int k = 0; k < k < 3 && k < dstNComps; ++k) {

                        if (dstNComps == 4) {
                            // For alpha channel, fill with 1, we reach here only if converting RGB-->RGBA or XY--->RGBA
                            dstPixels[3] =  convertPixelDepth<float, DSTPIX>(useAlpha0 ? 0.f : 1.f);
                        }
                    } // if (srcNComps == 1) {
                } // if (dstNComps == 1) {
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

    if (copyBitmap) {
        dstImg.copyBitmapPortion(renderWindow, srcImg);
    }
} // Image::convertToFormatInternalForColorSpace

template <typename SRCPIX, typename DSTPIX, int srcMaxValue, int dstMaxValue, int srcNComps, int dstNComps,
          bool requiresUnpremult>
void
Image::convertToFormatInternalForUnpremult(const RectI & renderWindow,
                                           const Image & srcImg,
                                           Image & dstImg,
                                           ViewerColorSpaceEnum srcColorSpace,
                                           ViewerColorSpaceEnum dstColorSpace,
                                           bool useAlpha0,
                                           bool copyBitmap,
                                           int channelForAlpha)
{
    if ( (srcColorSpace == eViewerColorSpaceLinear) && (dstColorSpace == eViewerColorSpaceLinear) ) {
        convertToFormatInternalForColorSpace<SRCPIX, DSTPIX, srcMaxValue, dstMaxValue, srcNComps, dstNComps, requiresUnpremult, false>(renderWindow, srcImg, dstImg, copyBitmap, useAlpha0, srcColorSpace, dstColorSpace, channelForAlpha);
    } else {
        convertToFormatInternalForColorSpace<SRCPIX, DSTPIX, srcMaxValue, dstMaxValue, srcNComps, dstNComps, requiresUnpremult, true>(renderWindow, srcImg, dstImg, copyBitmap, useAlpha0, srcColorSpace, dstColorSpace, channelForAlpha);
    }
}

template <typename SRCPIX, typename DSTPIX, int srcMaxValue, int dstMaxValue, int srcNComps, int dstNComps>
void
Image::convertToFormatInternal(const RectI & renderWindow,
                               const Image & srcImg,
                               Image & dstImg,
                               ViewerColorSpaceEnum srcColorSpace,
                               ViewerColorSpaceEnum dstColorSpace,
                               int channelForAlpha,
                               bool useAlpha0,
                               bool copyBitmap,
                               bool requiresUnpremult)
{
    if (requiresUnpremult) {
        convertToFormatInternalForUnpremult<SRCPIX, DSTPIX, srcMaxValue, dstMaxValue, srcNComps, dstNComps, true>(renderWindow, srcImg, dstImg, srcColorSpace, dstColorSpace, useAlpha0, copyBitmap, channelForAlpha);
    } else {
        convertToFormatInternalForUnpremult<SRCPIX, DSTPIX, srcMaxValue, dstMaxValue, srcNComps, dstNComps, false>(renderWindow, srcImg, dstImg, srcColorSpace, dstColorSpace, useAlpha0, copyBitmap, channelForAlpha);
    }
}

template <typename SRCPIX, typename DSTPIX, int srcMaxValue, int dstMaxValue>
void
Image::convertToFormatInternalForDepth(const RectI & renderWindow,
                                       const Image & srcImg,
                                       Image & dstImg,
                                       ViewerColorSpaceEnum srcColorSpace,
                                       ViewerColorSpaceEnum dstColorSpace,
                                       int channelForAlpha,
                                       bool useAlpha0,
                                       bool copyBitmap,
                                       bool requiresUnpremult)
{
    int dstNComp = dstImg.getComponents().getNumComponents();
    int srcNComp = srcImg.getComponents().getNumComponents();

    if (requiresUnpremult) {
        // see convertToFormatInternalForColorSpace : it is only used in one case!
        assert(srcNComp == 4 &&
               dstNComp == 3);
    }

    switch (srcNComp) {
    case 1:
        switch (dstNComp) {
        case 2:
            convertToFormatInternal<SRCPIX, DSTPIX, srcMaxValue, dstMaxValue, 1, 2>(renderWindow, srcImg, dstImg,
                                                                                    srcColorSpace,
                                                                                    dstColorSpace,
                                                                                    channelForAlpha,
                                                                                    useAlpha0,
                                                                                    copyBitmap,
                                                                                    /*requiresUnpremult=*/ false);
            break;
        case 3:
            convertToFormatInternal<SRCPIX, DSTPIX, srcMaxValue, dstMaxValue, 1, 3>(renderWindow, srcImg, dstImg,
                                                                                    srcColorSpace,
                                                                                    dstColorSpace,
                                                                                    channelForAlpha,
                                                                                    useAlpha0,
                                                                                    copyBitmap,
                                                                                    /*requiresUnpremult=*/ false);
            break;
        case 4:
            convertToFormatInternal<SRCPIX, DSTPIX, srcMaxValue, dstMaxValue, 1, 4>(renderWindow, srcImg, dstImg,
                                                                                    srcColorSpace,
                                                                                    dstColorSpace,
                                                                                    channelForAlpha,
                                                                                    useAlpha0,
                                                                                    copyBitmap,
                                                                                    /*requiresUnpremult=*/ false);
            break;
        default:
            assert(false);
            break;
        }
        break;
    case 2:
        switch (dstNComp) {
        case 1:
            convertToFormatInternal<SRCPIX, DSTPIX, srcMaxValue, dstMaxValue, 2, 1>(renderWindow, srcImg, dstImg,
                                                                                    srcColorSpace,
                                                                                    dstColorSpace,
                                                                                    channelForAlpha,
                                                                                    useAlpha0,
                                                                                    copyBitmap,
                                                                                    /*requiresUnpremult=*/ false);
            break;
        case 3:
            convertToFormatInternal<SRCPIX, DSTPIX, srcMaxValue, dstMaxValue, 2, 3>(renderWindow, srcImg, dstImg,
                                                                                    srcColorSpace,
                                                                                    dstColorSpace,
                                                                                    channelForAlpha,
                                                                                    useAlpha0,
                                                                                    copyBitmap,
                                                                                    /*requiresUnpremult=*/ false);
            break;
        case 4:
            convertToFormatInternal<SRCPIX, DSTPIX, srcMaxValue, dstMaxValue, 2, 4>(renderWindow, srcImg, dstImg,
                                                                                    srcColorSpace,
                                                                                    dstColorSpace,
                                                                                    channelForAlpha,
                                                                                    useAlpha0,
                                                                                    copyBitmap,
                                                                                    /*requiresUnpremult=*/ false);
            break;
        default:
            assert(false);
            break;
        }
        break;
    case 3:
        switch (dstNComp) {
        case 1:
            convertToFormatInternal<SRCPIX, DSTPIX, srcMaxValue, dstMaxValue, 3, 1>(renderWindow, srcImg, dstImg,
                                                                                    srcColorSpace,
                                                                                    dstColorSpace,
                                                                                    channelForAlpha,
                                                                                    useAlpha0,
                                                                                    copyBitmap,
                                                                                    /*requiresUnpremult=*/ false);
            break;
        case 2:
            convertToFormatInternal<SRCPIX, DSTPIX, srcMaxValue, dstMaxValue, 3, 2>(renderWindow, srcImg, dstImg,
                                                                                    srcColorSpace,
                                                                                    dstColorSpace,
                                                                                    channelForAlpha,
                                                                                    useAlpha0,
                                                                                    copyBitmap,
                                                                                    /*requiresUnpremult=*/ false);
            break;
        case 4:
            convertToFormatInternal<SRCPIX, DSTPIX, srcMaxValue, dstMaxValue, 3, 4>(renderWindow, srcImg, dstImg,
                                                                                    srcColorSpace,
                                                                                    dstColorSpace,
                                                                                    channelForAlpha,
                                                                                    useAlpha0,
                                                                                    copyBitmap,
                                                                                    /*requiresUnpremult=*/ false);
            break;
        default:
            assert(false);
            break;
        }
        break;
    case 4:
        switch (dstNComp) {
        case 1:
            convertToFormatInternal<SRCPIX, DSTPIX, srcMaxValue, dstMaxValue, 4, 1>(renderWindow, srcImg, dstImg,
                                                                                    srcColorSpace,
                                                                                    dstColorSpace,
                                                                                    channelForAlpha,
                                                                                    useAlpha0,
                                                                                    copyBitmap,
                                                                                    /*requiresUnpremult=*/ false);
            break;
        case 2:
            convertToFormatInternal<SRCPIX, DSTPIX, srcMaxValue, dstMaxValue, 4, 2>(renderWindow, srcImg, dstImg,
                                                                                    srcColorSpace,
                                                                                    dstColorSpace,
                                                                                    channelForAlpha,
                                                                                    useAlpha0,
                                                                                    copyBitmap,
                                                                                    /*requiresUnpremult=*/ false);
            break;
        case 3:
            convertToFormatInternal<SRCPIX, DSTPIX, srcMaxValue, dstMaxValue, 4, 3>(renderWindow, srcImg, dstImg,
                                                                                    srcColorSpace,
                                                                                    dstColorSpace,
                                                                                    channelForAlpha,
                                                                                    useAlpha0,
                                                                                    copyBitmap,
                                                                                    requiresUnpremult);       // only case where requiresUnpremult seems to be useful
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
Image::convertToFormatCommon(const RectI & renderWindow,
                             ViewerColorSpaceEnum srcColorSpace,
                             ViewerColorSpaceEnum dstColorSpace,
                             int channelForAlpha,
                             bool useAlpha0,
                             bool copyBitmap,
                             bool requiresUnpremult,
                             Image* dstImg) const
{
    QWriteLocker k(&dstImg->_entryLock);
    QReadLocker k2(&_entryLock);

    assert( _bounds.contains(renderWindow) &&  dstImg->_bounds.contains(renderWindow) );

    if ( dstImg->getComponents().getNumComponents() == getComponents().getNumComponents() ) {
        switch ( dstImg->getBitDepth() ) {
        case eImageBitDepthByte: {
            switch ( getBitDepth() ) {
            case eImageBitDepthByte:
                ///Same as a copy
                convertToFormatInternal_sameComps<unsigned char, unsigned char, 255, 255>(renderWindow, *this, *dstImg,
                                                                                          srcColorSpace,
                                                                                          dstColorSpace, copyBitmap);
                break;
            case eImageBitDepthShort:
                convertToFormatInternal_sameComps<unsigned short, unsigned char, 65535, 255>(renderWindow, *this, *dstImg,
                                                                                             srcColorSpace,
                                                                                             dstColorSpace, copyBitmap);
                break;
            case eImageBitDepthHalf:
                break;
            case eImageBitDepthFloat:
                convertToFormatInternal_sameComps<float, unsigned char, 1, 255>(renderWindow, *this, *dstImg,
                                                                                srcColorSpace,
                                                                                dstColorSpace, copyBitmap);
                break;
            case eImageBitDepthNone:
                break;
            }
            break;
        }

        case eImageBitDepthShort: {
            switch ( getBitDepth() ) {
            case eImageBitDepthByte:
                convertToFormatInternal_sameComps<unsigned char, unsigned short, 255, 65535>(renderWindow, *this, *dstImg,
                                                                                             srcColorSpace,
                                                                                             dstColorSpace, copyBitmap);
                break;
            case eImageBitDepthShort:
                ///Same as a copy
                convertToFormatInternal_sameComps<unsigned short, unsigned short, 65535, 65535>(renderWindow, *this, *dstImg,
                                                                                                srcColorSpace,
                                                                                                dstColorSpace, copyBitmap);
                break;
            case eImageBitDepthHalf:
                break;
            case eImageBitDepthFloat:
                convertToFormatInternal_sameComps<float, unsigned short, 1, 65535>(renderWindow, *this, *dstImg,
                                                                                   srcColorSpace,
                                                                                   dstColorSpace, copyBitmap);
                break;
            case eImageBitDepthNone:
                break;
            }
            break;
        }

        case eImageBitDepthHalf:
            break;

        case eImageBitDepthFloat: {
            switch ( getBitDepth() ) {
            case eImageBitDepthByte:
                convertToFormatInternal_sameComps<unsigned char, float, 255, 1>(renderWindow, *this, *dstImg,
                                                                                srcColorSpace,
                                                                                dstColorSpace, copyBitmap);
                break;
            case eImageBitDepthShort:
                convertToFormatInternal_sameComps<unsigned short, float, 65535, 1>(renderWindow, *this, *dstImg,
                                                                                   srcColorSpace,
                                                                                   dstColorSpace, copyBitmap);
                break;
            case eImageBitDepthHalf:
                break;
            case eImageBitDepthFloat:
                ///Same as a copy
                convertToFormatInternal_sameComps<float, float, 1, 1>(renderWindow, *this, *dstImg,
                                                                      srcColorSpace,
                                                                      dstColorSpace, copyBitmap);
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
        switch ( dstImg->getBitDepth() ) {
        case eImageBitDepthByte: {
            switch ( getBitDepth() ) {
            case eImageBitDepthByte:
                convertToFormatInternalForDepth<unsigned char, unsigned char, 255, 255>(renderWindow, *this, *dstImg,
                                                                                        srcColorSpace,
                                                                                        dstColorSpace,
                                                                                        channelForAlpha,
                                                                                        useAlpha0,
                                                                                        copyBitmap, requiresUnpremult);
                break;
            case eImageBitDepthShort:
                convertToFormatInternalForDepth<unsigned short, unsigned char, 65535, 255>(renderWindow, *this, *dstImg,
                                                                                           srcColorSpace,
                                                                                           dstColorSpace,
                                                                                           channelForAlpha,
                                                                                           useAlpha0,
                                                                                           copyBitmap, requiresUnpremult);
                break;
            case eImageBitDepthHalf:
                break;
            case eImageBitDepthFloat:
                convertToFormatInternalForDepth<float, unsigned char, 1, 255>(renderWindow, *this, *dstImg,
                                                                              srcColorSpace,
                                                                              dstColorSpace,
                                                                              channelForAlpha,
                                                                              useAlpha0,
                                                                              copyBitmap, requiresUnpremult);

                break;
            case eImageBitDepthNone:
                break;
            }
            break;
        }
        case eImageBitDepthShort: {
            switch ( getBitDepth() ) {
            case eImageBitDepthByte:
                convertToFormatInternalForDepth<unsigned char, unsigned short, 255, 65535>(renderWindow, *this, *dstImg,
                                                                                           srcColorSpace,
                                                                                           dstColorSpace,
                                                                                           channelForAlpha,
                                                                                           useAlpha0,
                                                                                           copyBitmap, requiresUnpremult);

                break;
            case eImageBitDepthShort:
                convertToFormatInternalForDepth<unsigned short, unsigned short, 65535, 65535>(renderWindow, *this, *dstImg,
                                                                                              srcColorSpace,
                                                                                              dstColorSpace,
                                                                                              channelForAlpha,
                                                                                              useAlpha0,
                                                                                              copyBitmap, requiresUnpremult);

                break;
            case eImageBitDepthHalf:
                break;
            case eImageBitDepthFloat:
                convertToFormatInternalForDepth<float, unsigned short, 1, 65535>(renderWindow, *this, *dstImg,
                                                                                 srcColorSpace,
                                                                                 dstColorSpace,
                                                                                 channelForAlpha,
                                                                                 useAlpha0,
                                                                                 copyBitmap, requiresUnpremult);
                break;
            case eImageBitDepthNone:
                break;
            }
            break;
        }
        case eImageBitDepthHalf:
            break;
        case eImageBitDepthFloat: {
            switch ( getBitDepth() ) {
            case eImageBitDepthByte:
                convertToFormatInternalForDepth<unsigned char, float, 255, 1>(renderWindow, *this, *dstImg,
                                                                              srcColorSpace,
                                                                              dstColorSpace,
                                                                              channelForAlpha,
                                                                              useAlpha0,
                                                                              copyBitmap, requiresUnpremult);
                break;
            case eImageBitDepthShort:
                convertToFormatInternalForDepth<unsigned short, float, 65535, 1>(renderWindow, *this, *dstImg,
                                                                                 srcColorSpace,
                                                                                 dstColorSpace,
                                                                                 channelForAlpha,
                                                                                 useAlpha0,
                                                                                 copyBitmap, requiresUnpremult);

                break;
            case eImageBitDepthHalf:
                break;
            case eImageBitDepthFloat:
                convertToFormatInternalForDepth<float, float, 1, 1>(renderWindow, *this, *dstImg,
                                                                    srcColorSpace,
                                                                    dstColorSpace,
                                                                    channelForAlpha,
                                                                    useAlpha0,
                                                                    copyBitmap, requiresUnpremult);
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
} // Image::convertToFormatCommon

void
Image::convertToFormat(const RectI & renderWindow,
                       ViewerColorSpaceEnum srcColorSpace,
                       ViewerColorSpaceEnum dstColorSpace,
                       int channelForAlpha,
                       bool copyBitmap,
                       bool requiresUnpremult,
                       Image* dstImg) const
{
    // OpenGL textures are always RGBA anyway
    assert(getStorageMode() != eStorageModeGLTex);
    convertToFormatCommon(renderWindow, srcColorSpace, dstColorSpace, channelForAlpha, false, copyBitmap, requiresUnpremult, dstImg);
} // convertToFormat

void
Image::convertToFormatAlpha0(const RectI & renderWindow,
                             ViewerColorSpaceEnum srcColorSpace,
                             ViewerColorSpaceEnum dstColorSpace,
                             int channelForAlpha,
                             bool copyBitmap,
                             bool requiresUnpremult,
                             Image* dstImg) const
{
    assert(getStorageMode() != eStorageModeGLTex);
    convertToFormatCommon(renderWindow, srcColorSpace, dstColorSpace, channelForAlpha, true, copyBitmap, requiresUnpremult, dstImg);
}

NATRON_NAMESPACE_EXIT
