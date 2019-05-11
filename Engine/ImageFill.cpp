/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
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
#include "Engine/Texture.h"
NATRON_NAMESPACE_ENTER

template <typename GL>
static void
fillGLInternal(const RectI & roi,
               float r,
               float g,
               float b,
               float a,
               const GLTexturePtr& texture,
               const OSGLContextPtr& glContext)
{
    RectI bounds = texture->getBounds();

    RectI realRoi;
    roi.intersect(bounds, &realRoi);

    GLuint fboID = glContext->getOrCreateFBOId();

    int target = texture->getTexTarget();
    U32 texID = texture->getTexID();


    GL::BindFramebuffer(GL_FRAMEBUFFER, fboID);
    GL::Enable(target);
    GL::ActiveTexture(GL_TEXTURE0);
    GL::BindTexture( target, texID );

    GL::TexParameteri (target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    GL::TexParameteri (target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    GL::TexParameteri (target, GL_TEXTURE_WRAP_S, GL_REPEAT);
    GL::TexParameteri (target, GL_TEXTURE_WRAP_T, GL_REPEAT);

    GL::FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, target, texID, 0 /*LoD*/);
    glCheckFramebufferError(GL);

    OSGLContext::setupGLViewport<GL>(bounds, realRoi);
    GL::ClearColor(r, g, b, a);
    GL::Clear(GL_COLOR_BUFFER_BIT);

    GL::BindTexture(target, 0);
    glCheckError(GL);

} // fillGLInternal

void
ImagePrivate::fillGL(const RectI & roi,
                     float r,
                     float g,
                     float b,
                     float a,
                     const GLTexturePtr& texture,
                     const OSGLContextPtr& glContext)
{

    // Save the current context
    OSGLContextSaver saveCurrentContext;

    {
        // Ensure this context is attached
        OSGLContextAttacherPtr contextAttacher = OSGLContextAttacher::create(glContext);
        contextAttacher->attach();


        if (glContext->isGPUContext()) {
            fillGLInternal<GL_GPU>(roi, r, g, b, a, texture, glContext);
        } else {
            fillGLInternal<GL_CPU>(roi, r, g, b, a, texture, glContext);
        }
    }

    // th old context will be restored when saveCurrentContext is destroyed
} // fillGL

template <typename PIX>
ActionRetCodeEnum
fillCPUBlackForDepth(void* ptrs[4],
                     int nComps,
                     const RectI& bounds,
                     const RectI& roi,
                     const EffectInstancePtr& renderClone)
{
    int dataSizeOf = sizeof(PIX);
    
    // memset for each scan-line
    for (int y = roi.y1; y < roi.y2; ++y) {

        if (renderClone && renderClone->isRenderAborted()) {
            return eActionStatusAborted;
        }

        PIX* dstPixelPtrs[4] = { NULL, NULL, NULL, NULL};
        int dstPixelStride;
        Image::getChannelPointers<PIX>((const PIX**)ptrs, roi.x1, y, bounds, nComps, dstPixelPtrs, &dstPixelStride);

        // check if all required components are present and contiguous
        bool contiguous = true;
        if (dstPixelStride != nComps) {
            contiguous = false;
        }
        for (int c = 0; c < nComps; ++c) {
            if (!dstPixelPtrs[c]) {
                contiguous = false;
            }
        }
        for (int c = 1; c < nComps; ++c) {
            if (dstPixelPtrs[c] - dstPixelPtrs[c-1] != 1) {
                contiguous = false;
            }
        }
        if (contiguous) {
            // If all channels belong to the same buffer, use memset
            std::size_t rowSize = roi.width() * dstPixelStride * dataSizeOf;
            memset(dstPixelPtrs[0], 0, rowSize);
        } else {
            for (int i = 0; i < 4; ++i) {
                if (dstPixelPtrs[i]) {
                    for (int x = roi.x1; x < roi.x2; ++x, dstPixelPtrs[i] += dstPixelStride) {
                        *dstPixelPtrs[i] = 0;
                    }
                }
            }
        }

    }
    return eActionStatusOK;
}

static ActionRetCodeEnum
fillCPUBlack(void* ptrs[4],
             int nComps,
             ImageBitDepthEnum bitDepth,
             const RectI& bounds,
             const RectI& roi,
             const EffectInstancePtr& renderClone)
{
    // memset the whole bounds at once if we can.
    if (roi == bounds && !ptrs[1]) {
        std::size_t planeSize = nComps * bounds.area() * getSizeOfForBitDepth(bitDepth);
        memset(ptrs[0], 0, planeSize);
    } else {
        switch (bitDepth) {
            case eImageBitDepthByte:
                return fillCPUBlackForDepth<unsigned char>(ptrs, nComps, bounds, roi, renderClone);
            case eImageBitDepthShort:
                return fillCPUBlackForDepth<unsigned short>(ptrs, nComps, bounds, roi, renderClone);
            case eImageBitDepthFloat:
                return fillCPUBlackForDepth<float>(ptrs, nComps, bounds, roi, renderClone);
            default:
                return eActionStatusFailed;
        }

    }

    return eActionStatusOK;
} // fillCPUBlack

template <typename PIX, int maxValue, int nComps>
static ActionRetCodeEnum
fillForDepthForComponents(void* ptrs[4],
                          float r,
                          float g,
                          float b,
                          float a,
                          const RectI& bounds,
                          const RectI& roi,
                          const EffectInstancePtr& renderClone)
{
    const float fillValue[4] = {
        nComps == 1 ? a * maxValue : r * maxValue, g * maxValue, b * maxValue, a * maxValue
    };

    int nCompsPerBuffer = nComps;
    if (nComps > 1 && ptrs[1]) {
        nCompsPerBuffer = 1;
    }


    // now we're safe: the image contains the area in roi
    PIX* dstPixelPtrs[4] = {NULL, NULL, NULL, NULL};
    int dstPixelStride;
    Image::getChannelPointers<PIX>((const PIX**)ptrs, roi.x1, roi.y1, bounds, nComps, (PIX**)dstPixelPtrs, &dstPixelStride);

    std::size_t nElementsPerRow = nCompsPerBuffer * dstPixelStride;


    for (int y = roi.y1; y < roi.y2; ++y) {

        if (renderClone && renderClone->isRenderAborted()) {
            return eActionStatusAborted;
        }

        for (int x = roi.x1; x < roi.x2; ++x) {
            for (int c = 0; c < 4; ++c) {
                if (dstPixelPtrs[c]) {
                    *dstPixelPtrs[c] = fillValue[c];
                    dstPixelPtrs[c] += dstPixelStride;
                }
            }
        }
        for (int c = 0; c < 4; ++c) {
            if (dstPixelPtrs[c]) {
                dstPixelPtrs[c] += nElementsPerRow - roi.width() * dstPixelStride;
            }
        }
    }
    return eActionStatusOK;
} // fillForDepthForComponents


template <typename PIX, int maxValue>
static ActionRetCodeEnum
fillForDepth(void* ptrs[4],
             float r,
             float g,
             float b,
             float a,
             int nComps,
             const RectI& bounds,
             const RectI& roi,
             const EffectInstancePtr& renderClone)
{
    switch (nComps) {
        case 1:
            return fillForDepthForComponents<PIX, maxValue, 1>(ptrs, r, g, b, a, bounds, roi, renderClone);
        case 2:
            return fillForDepthForComponents<PIX, maxValue, 2>(ptrs, r, g, b, a, bounds, roi, renderClone);
        case 3:
            return fillForDepthForComponents<PIX, maxValue, 3>(ptrs, r, g, b, a, bounds, roi, renderClone);
        case 4:
            return fillForDepthForComponents<PIX, maxValue, 4>(ptrs, r, g, b, a, bounds, roi, renderClone);
        default:
            break;
    }
    return eActionStatusFailed;
} // fillForDepth


ActionRetCodeEnum
ImagePrivate::fillCPU(void* ptrs[4],
                      float r,
                      float g,
                      float b,
                      float a,
                      int nComps,
                      ImageBitDepthEnum bitDepth,
                      const RectI& bounds,
                      const RectI& roi,
                      const EffectInstancePtr& renderClone)
{
    if (r == 0 && g == 0 &&  b == 0 && a == 0) {
        return fillCPUBlack(ptrs, nComps, bitDepth, bounds, roi, renderClone);
    }

    switch (bitDepth) {
        case eImageBitDepthByte:
            return fillForDepth<unsigned char, 255>(ptrs, r, g, b, a, nComps, bounds, roi, renderClone);
        case eImageBitDepthFloat:
            return fillForDepth<float, 1>(ptrs, r, g, b, a, nComps, bounds, roi, renderClone);
        case eImageBitDepthShort:
            return fillForDepth<unsigned short, 65535>(ptrs, r, g, b, a, nComps, bounds, roi, renderClone);
        case eImageBitDepthHalf:
        default:
            break;
    }
    return eActionStatusFailed;


} // fillCPU

NATRON_NAMESPACE_EXIT
