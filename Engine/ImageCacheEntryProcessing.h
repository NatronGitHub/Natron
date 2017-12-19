/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2013-2017 INRIA and Alexandre Gauthier-Foichat
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

#ifndef IMAGECACHEENTRYPROCESSING_H
#define IMAGECACHEENTRYPROCESSING_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#if !defined(SBK_RUN) && !defined(Q_MOC_RUN)
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_OFF
#include <boost/math/special_functions/fpclassify.hpp>
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_ON
#endif

#include "Engine/EngineFwd.h"
#include "Engine/RectI.h"
#include "Global/GlobalDefines.h"

NATRON_NAMESPACE_ENTER

class ImageCacheEntryProcessing
{
public:

    ImageCacheEntryProcessing();


template <typename PIX>
static void copyPixelsForDepth(const RectI& renderWindow,
                        const PIX* srcPixelsData,
                        int srcXStride,
                        int srcYStride,
                        PIX* dstPixelsData,
                        int dstXStride,
                        int dstYStride)
{
    const PIX* src_pixels = srcPixelsData;
    PIX* dst_pixels = dstPixelsData;

    if (srcXStride == 1 && srcXStride == dstXStride) {
        for (int y = renderWindow.y1; y < renderWindow.y2; ++y,
             src_pixels += srcYStride,
             dst_pixels += dstYStride) {

            memcpy(dst_pixels, src_pixels, renderWindow.width() * sizeof(PIX));
#ifdef DEBUG
            const PIX* p = src_pixels;
            for (int x = renderWindow.x1; x < renderWindow.x2; ++x, p += srcXStride) {
                assert( !(boost::math::isnan)(*p) ); // NaN check
            }
#endif
        }
    } else {
        for (int y = renderWindow.y1; y < renderWindow.y2; ++y) {
            for (int x = renderWindow.x1; x < renderWindow.x2; ++x,
                 src_pixels += srcXStride,
                 dst_pixels += dstXStride) {
                assert( !(boost::math::isnan)(*src_pixels) ); // NaN check
                *dst_pixels = *src_pixels;
            }

            dst_pixels += (dstYStride - renderWindow.width() * dstXStride);
            src_pixels += (srcYStride - renderWindow.width() * srcXStride);
        }
    }
} // copyPixelsForDepth

static void copyPixels(const RectI& renderWindow,
                       ImageBitDepthEnum depth,
                       const void* srcPixelsData,
                       int srcXStride,
                       int srcYStride,
                       void* dstPixelsData,
                       int dstXStride,
                       int dstYStride)
{
    switch (depth) {
        case eImageBitDepthByte:
            copyPixelsForDepth<char>(renderWindow, (const char*)srcPixelsData, srcXStride, srcYStride, (char*)dstPixelsData, dstXStride, dstYStride);
            break;
        case eImageBitDepthShort:
            copyPixelsForDepth<unsigned short>(renderWindow, (const unsigned short*)srcPixelsData, srcXStride, srcYStride, (unsigned short*)dstPixelsData, dstXStride, dstYStride);
            break;
        case eImageBitDepthFloat:
            copyPixelsForDepth<float>(renderWindow, (const float*)srcPixelsData, srcXStride, srcYStride, (float*)dstPixelsData, dstXStride, dstYStride);
            break;
        default:
            break;
    }
}


template <typename PIX>
static PIX* getPix(PIX* ptr,
            int x, int y,
            const RectI& bounds)
{
    if (x < bounds.x1 || x >= bounds.x2 || y < bounds.y1 || y >= bounds.y2) {
        return 0;
    }
    return ptr + bounds.width() * (y - bounds.y1) + (x - bounds.x1);
}

template <typename PIX>
static void fillWithConstant(PIX val,
                             PIX* ptr,
                             const RectI& roi,
                             const RectI& bounds)
{

    assert( !(boost::math::isnan)(val) ); // NaN check
    PIX* pix = getPix(ptr, roi.x1, roi.y1, bounds);
    for (int y = roi.y1; y < roi.y2; ++y) {
        for (int x = roi.x1; x < roi.x2; ++x) {
            *pix = val;
            ++pix;
        }
        pix += (bounds.width() - roi.width());
    }
}
    
/**
 * @brief If the tile bounds is not aligned to the tile size, repeat pixels on the edge.
 * This function fills the numbered rectangles with the nearest pixel V as such:

 12223
 4VVV5
 4VVV5
 67778
 **/
template <typename PIX>
static void repeatEdgesForDepth(PIX* ptr,
                                const RectI& bounds,
                                int tileSizeX,
                                int tileSizeY)
{

    assert(bounds.width() <= tileSizeX);
    assert(bounds.height() <= tileSizeY);

    RectI roundedBounds = bounds;
    roundedBounds.roundToTileSize(tileSizeX, tileSizeY);
    assert(roundedBounds.width() == tileSizeX);
    assert(roundedBounds.height() == tileSizeY);

    {
        // 1
        RectI roi;
        roi.set(roundedBounds.x1, bounds.y2, bounds.x1, roundedBounds.y2);
        if (!roi.isNull()) {
            PIX val = *getPix(ptr, bounds.x1, bounds.y2 - 1, roundedBounds);
            fillWithConstant(val, ptr, roi, roundedBounds);
        }
    }
    {
        // 2
        RectI roi;
        roi.set(bounds.x1, bounds.y2, bounds.x2, roundedBounds.y2);
        if (!roi.isNull()) {

            PIX* origPix = getPix(ptr, bounds.x1, bounds.y2 - 1, roundedBounds);

            PIX* pix = getPix(ptr, roi.x1, roi.y1, roundedBounds);
            for (int y = roi.y1; y < roi.y2; ++y) {
                PIX* srcPix = origPix;
                for (int x = roi.x1; x < roi.x2; ++x) {
                    assert( !(boost::math::isnan)(*srcPix) ); // NaN check
                    *pix = *srcPix;
                    ++pix;
                    ++srcPix;
                }
                // Remove what was done in the last iteration and go to the next line
                pix += (tileSizeX - roi.width());
            }
        }
    }
    {
        // 3
        RectI roi;
        roi.set(bounds.x2, bounds.y2, roundedBounds.x2, roundedBounds.y2);
        if (!roi.isNull()) {
            PIX val = *getPix(ptr, bounds.x2 - 1, bounds.y2 - 1, roundedBounds);
            fillWithConstant(val, ptr, roi, roundedBounds);
        }
    }
    {
        // 4
        RectI roi;
        roi.set(roundedBounds.x1, bounds.y1, bounds.x1, bounds.y2);
        if (!roi.isNull()) {
            PIX* dst_pix = getPix(ptr, roi.x1, roi.y1, roundedBounds);
            for (int y = roi.y1; y < roi.y2; ++y) {
                PIX val = *getPix(ptr, bounds.x1, y, roundedBounds);
                assert( !(boost::math::isnan)(val) ); // NaN check
                for (int x = roi.x1; x < roi.x2; ++x) {
                    *dst_pix = val;
                    ++dst_pix;
                }
                // Remove what was done in the last iteration and go to the next line
                dst_pix += (tileSizeX - roi.width());
            }
        }
    }
    {
        // 5
        RectI roi;
        roi.set(bounds.x2, bounds.y1, roundedBounds.x2, bounds.y2);
        if (!roi.isNull()) {
            PIX* pix = getPix(ptr, roi.x1, roi.y1, roundedBounds);
            for (int y = roi.y1; y < roi.y2; ++y) {
                PIX val = *getPix(ptr, bounds.x2 - 1, y, roundedBounds);
                assert( !(boost::math::isnan)(val) ); // NaN check
                for (int x = roi.x1; x < roi.x2; ++x) {
                    *pix = val;
                    ++pix;
                }
                // Remove what was done in the last iteration and go to the next line
                pix += (tileSizeX - roi.width());
            }
        }
    }
    {
        // 6
        RectI roi;
        roi.set(roundedBounds.x1, roundedBounds.y1, bounds.x1, bounds.y1);
        if (!roi.isNull()) {
            PIX val = *getPix(ptr, bounds.x1, bounds.y1, roundedBounds);
            fillWithConstant(val, ptr, roi, roundedBounds);
        }
    }
    {
        // 7
        RectI roi;
        roi.set(bounds.x1, roundedBounds.y1, bounds.x2, bounds.y1);
        if (!roi.isNull()) {

            PIX* origPix = getPix(ptr, bounds.x1, bounds.y1, roundedBounds);

            PIX* dst_pix = getPix(ptr, roi.x1, roi.y1, roundedBounds);
            for (int y = roi.y1; y < roi.y2; ++y) {
                PIX* srcPix = origPix;
                for (int x = roi.x1; x < roi.x2; ++x) {
                    assert( !(boost::math::isnan)(*srcPix) ); // NaN check
                    *dst_pix = *srcPix;
                    ++dst_pix;
                    ++srcPix;
                }
                // Remove what was done in the last iteration and go to the next line
                dst_pix += (tileSizeX - roi.width());
            }
        }
    }
    {
        // 8
        RectI roi;
        roi.set(bounds.x2, roundedBounds.y1, roundedBounds.x2, bounds.y1);
        if (!roi.isNull()) {
            PIX val = *getPix(ptr, bounds.x2 - 1, bounds.y1, roundedBounds);
            fillWithConstant(val, ptr, roi, roundedBounds);
        }
    }

} // repeatEdgesForDepth

static void repeatEdges(ImageBitDepthEnum depth,
                        void* ptr,
                        const RectI& bounds,
                        int tileSizeX,
                        int tileSizeY)
{
    switch (depth) {
        case eImageBitDepthByte:
            repeatEdgesForDepth<char>((char*)ptr, bounds, tileSizeX, tileSizeY);
            break;
        case eImageBitDepthShort:
            repeatEdgesForDepth<unsigned short>((unsigned short*)ptr, bounds, tileSizeX, tileSizeY);
            break;
        case eImageBitDepthFloat:
            repeatEdgesForDepth<float>((float*)ptr, bounds, tileSizeX, tileSizeY);
            break;
        default:
            break;
    }
} // repeatEdges



template <typename PIX>
static void downscaleMipMapForDepth(const PIX* srcTilesPtr[4],
                                    PIX* dstTilePtr,
                                    const RectI& dstTileBounds,
                                    int tileSizeX,
                                    int tileSizeY)
{
    // All tiles have the same bounds: we don't care about coordinates in this case nor checking whether we are in the bounds

    // Since in input some tiles may be invalid, we keep track of the area we filled
    // In input we either have 0, 2 or 3 tiles invalid: 2 if we are a tile on the edge, 3 in the corner
    RectI filledBounds;
    RectI dstTileBoundsRounded = dstTileBounds;
    dstTileBoundsRounded.roundToTileSize(tileSizeX, tileSizeY);

    const int halfTileSizeX = tileSizeX / 2;
    const int halfTileSizeY = tileSizeY / 2;
#ifndef NDEBUG
    int nInvalid = 0;
#endif
    for (int ty = 0; ty < 2; ++ty) {
        for (int tx = 0; tx < 2; ++tx) {

            int t_i = ty * 2 + tx;
            if (!srcTilesPtr[t_i]) {
                // This tile will be filled in repeatEdgesForDepth
#ifndef NDEBUG
                ++nInvalid;
#endif
                continue;
            } else {
                RectI subTileRect;
                subTileRect.x1 = dstTileBoundsRounded.x1 + tx * halfTileSizeX;
                subTileRect.y1 = dstTileBoundsRounded.y1 + ty * halfTileSizeY;
                subTileRect.x2 = subTileRect.x1 + halfTileSizeX;
                subTileRect.y2 = subTileRect.y1 + halfTileSizeY;
                if (filledBounds.isNull()) {
                    filledBounds = subTileRect;
                } else {
                    filledBounds.merge(subTileRect);
                }
            }

            PIX* dst_pixels = dstTilePtr + (halfTileSizeY * ty * tileSizeX) + (halfTileSizeX * tx);

            const PIX* src_pixels = srcTilesPtr[t_i];
            const PIX* src_pixels_next = srcTilesPtr[t_i] + tileSizeX;

            for (int y = 0; y < halfTileSizeY; ++y) {
                for (int x = 0; x < halfTileSizeX; ++x) {
                    
                    assert( !(boost::math::isnan)(*src_pixels) ); // NaN check
                    assert( !(boost::math::isnan)(*(src_pixels + 1)) ); // NaN check
                    assert( !(boost::math::isnan)(*(src_pixels_next)) ); // NaN check
                    assert( !(boost::math::isnan)(*(src_pixels_next + 1)) ); // NaN check

                    double sum = (double)*src_pixels + (double)*(src_pixels + 1);
                    sum += ((double)*src_pixels_next + (double)*(src_pixels_next + 1));
                    sum /= 4;
                    *dst_pixels = (PIX)sum;

                    src_pixels_next += 2;
                    src_pixels += 2;
                    ++dst_pixels;
                }
                src_pixels += tileSizeX;
                src_pixels_next += tileSizeX;
                dst_pixels += halfTileSizeX;
            }
        }
    }

    assert(nInvalid == 0 || nInvalid == 2 || nInvalid == 3);


    if (filledBounds.width() != tileSizeX || filledBounds.height() != tileSizeY) {
        repeatEdgesForDepth<PIX>(dstTilePtr, filledBounds, tileSizeX, tileSizeY);
    }

} // downscaleMipMapForDepth

static void downscaleMipMap(ImageBitDepthEnum depth,
                            const void* srcTilesPtr[4],
                            void* dstTilePtr,
                            const RectI& dstTileBounds,
                            int tileSizeX,
                            int tileSizeY)
{
    switch (depth) {
        case eImageBitDepthByte:
            downscaleMipMapForDepth<char>((const char**)srcTilesPtr, (char*)dstTilePtr, dstTileBounds, tileSizeX, tileSizeY);
            break;
        case eImageBitDepthShort:
            downscaleMipMapForDepth<unsigned short>((const unsigned short**)srcTilesPtr, (unsigned short*)dstTilePtr, dstTileBounds, tileSizeX, tileSizeY);
            break;
        case eImageBitDepthFloat:
            downscaleMipMapForDepth<float>((const float**)srcTilesPtr, (float*)dstTilePtr, dstTileBounds, tileSizeX, tileSizeY);
            break;
        default:
            break;
    }
    
}

}; // ImageCacheEntryProcessing

NATRON_NAMESPACE_EXIT

#endif // IMAGECACHEENTRYPROCESSING_H
