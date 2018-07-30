/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * Copyright (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
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

NATRON_NAMESPACE_ENTER

template <typename PIX, int nComps>
ActionRetCodeEnum
applyCPUPixelShaderInternal(const RectI& roi,
                            const void* customData,
                            const EffectInstancePtr& renderClone,
                            const Image::CPUData& cpuData,
                            void (*func)(const void*, int, PIX*[4]))
{
    PIX* dstPixelPtrs[4] = {NULL, NULL, NULL, NULL};
    int dstPixelStride;
    Image::getChannelPointers<PIX, nComps>((const PIX**)cpuData.ptrs, roi.x1, roi.y1, cpuData.bounds, (PIX**)dstPixelPtrs, &dstPixelStride);

    const std::size_t dstRowElements = (std::size_t)cpuData.bounds.width() * dstPixelStride;

    for ( int y = roi.y1; y < roi.y2; ++y) {

        if (renderClone && renderClone->isRenderAborted()) {
            return eActionStatusAborted;
        }
        for (int x = roi.x1; x < roi.x2; ++x) {

            func(customData, nComps, dstPixelPtrs);

            for (int c = 0; c < nComps; ++c) {
                dstPixelPtrs[c] += dstPixelStride;
            }
        }
        for (int c = 0; c < nComps; ++c) {
            dstPixelPtrs[c] += (dstRowElements - roi.width() * dstPixelStride);
        }
    }
    return eActionStatusOK;
}


template <typename PIX>
ActionRetCodeEnum
applyCPUPixelShaderForNComps(const RectI& roi,
                             const void* customData,
                            const EffectInstancePtr& renderClone,
                            const Image::CPUData& cpuData,
                            void (*func)(const void*, int , PIX* [4]))
{
    switch (cpuData.nComps) {
        case 1:
            return applyCPUPixelShaderInternal<PIX,1>(roi, customData, renderClone, cpuData, func);
        case 2:
            return applyCPUPixelShaderInternal<PIX,2>(roi, customData, renderClone, cpuData, func);
        case 3:
            return applyCPUPixelShaderInternal<PIX,3>(roi, customData, renderClone, cpuData, func);
        case 4:
            return applyCPUPixelShaderInternal<PIX,4>(roi, customData, renderClone, cpuData, func);
        default:
            return eActionStatusFailed;
    }
}


ActionRetCodeEnum
ImagePrivate::applyCPUPixelShader(const RectI& roi,
                                  const void* customData,
                                  const EffectInstancePtr& renderClone,
                                  const Image::CPUData& cpuData,
                                  void(*func)() )
{
    switch (cpuData.bitDepth) {
        case eImageBitDepthByte:
            return applyCPUPixelShaderForNComps<unsigned char>(roi, customData, renderClone, cpuData, (Image::ImageCPUPixelShaderByte)func);
        case eImageBitDepthFloat:
            return applyCPUPixelShaderForNComps<float>(roi, customData, renderClone, cpuData, (Image::ImageCPUPixelShaderFloat)func);
        case eImageBitDepthShort:
            return applyCPUPixelShaderForNComps<unsigned short>(roi, customData, renderClone, cpuData, (Image::ImageCPUPixelShaderShort)func);
        default:
            return eActionStatusFailed;
    }
}


NATRON_NAMESPACE_EXIT
