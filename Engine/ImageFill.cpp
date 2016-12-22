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

NATRON_NAMESPACE_ENTER;

template <typename GL>
static void
fillGLInternal(const RectI & roi,
               float r,
               float g,
               float b,
               float a,
               const GLCacheEntryPtr& texture,
               const OSGLContextPtr& glContext)
{
    RectI bounds = texture->getBounds();

    RectI realRoi;
    roi.intersect(bounds, &realRoi);

    GLuint fboID = glContext->getOrCreateFBOId();

    int target = texture->getGLTextureTarget();
    U32 texID = texture->getGLTextureID();


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
                     const GLCacheEntryPtr& texture)
{

    OSGLContextPtr glContext = texture->getOpenGLContext();
    if (!glContext) {
        // No context - This is a bug, OpenGL contexts should live as long as the application lives, much longer than an image.
        assert(false);
        return;
    }

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

static void
fillCPUBlack(void* ptrs[4],
                           int nComps,
                           ImageBitDepthEnum bitDepth,
                           const RectI& bounds,
                           const RectI& roi,
                           const TreeRenderNodeArgsPtr& renderArgs)
{
    if (roi == bounds) {
        // memset the whole bounds at once
        int nCompsPerPixel = nComps;
        if (nComps > 1 && ptrs[1]) {
            // Co-planar buffers have 1 comp
            nCompsPerPixel = 1;
        }
        std::size_t planeSize = nCompsPerPixel * bounds.area() * getSizeOfForBitDepth(bitDepth);
        for (int i = 0; i < 4; ++i) {
            if (ptrs[i]) {
                memset(ptrs[i], 0, planeSize);
            }
        }
    } else {
        int dataSizeOf = getSizeOfForBitDepth(bitDepth);

        // memset for each scan-line
        for (int y = roi.y1; y < roi.y2; ++y) {

            if (renderArgs && renderArgs->isAborted()) {
                return;
            }

            char* dstPixelPtrs[4];
            int dstPixelStride;
            Image::getChannelPointers<char>((const char**)ptrs, roi.x1, y, bounds, nComps, dstPixelPtrs, &dstPixelStride);

            std::size_t rowSize = roi.width() * dstPixelStride * dataSizeOf;

            for (int i = 0; i < 4; ++i) {
                if (dstPixelPtrs[i]) {
                    memset(dstPixelPtrs[i], 0, rowSize);
                }
            }

        }
    }
} // fillCPUBlack

template <typename PIX, int maxValue, int nComps>
static void
fillForDepthForComponents(void* ptrs[4],
                          float r,
                          float g,
                          float b,
                          float a,
                          const RectI& bounds,
                          const RectI& roi,
                          const TreeRenderNodeArgsPtr& renderArgs)
{
    const float fillValue[4] = {
        nComps == 1 ? a * maxValue : r * maxValue, g * maxValue, b * maxValue, a * maxValue
    };

    int nCompsPerBuffer = nComps;
    if (nComps > 1 && ptrs[1]) {
        nCompsPerBuffer = 1;
    }

    std::size_t nElementsPerRow = nCompsPerBuffer * bounds.width();

    // now we're safe: the image contains the area in roi
    PIX* dstPixelPtrs[4];
    int dstPixelStride;
    Image::getChannelPointers<char>((PIX**)ptrs, roi.x1, roi.y1, bounds, nComps, dstPixelPtrs, &dstPixelStride);

    for (int y = roi.y1; y < roi.y2; ++y) {

        if (renderArgs && renderArgs->isAborted()) {
            return;
        }

        for (int x = roi.x1; x < roi.x2; ++x) {
            for (int c = 0; c < 4; ++c) {
                if (dstPixelPtrs[c]) {
                    *dstPixelPtrs[c] = fillValue[c];
                    ++dstPixelPtrs[c];
                }
            }
        }
        for (int c = 0; c < 4; ++c) {
            if (dstPixelPtrs[c]) {
                dstPixelPtrs[c] += nElementsPerRow - roi.width() * nCompsPerBuffer;
            }
        }
    }

} // fillForDepthForComponents


template <typename PIX, int maxValue>
static void
fillForDepth(void* ptrs[4],
             float r,
             float g,
             float b,
             float a,
             int nComps,
             const RectI& bounds,
             const RectI& roi,
             const TreeRenderNodeArgsPtr& renderArgs)
{
    switch (nComps) {
        case 1:
            fillForDepthForComponents<PIX, maxValue, 1>(ptrs, r, g, b, a, bounds, roi, renderArgs);
            break;
        case 2:
            fillForDepthForComponents<PIX, maxValue, 2>(ptrs, r, g, b, a, bounds, roi, renderArgs);
            break;
        case 3:
            fillForDepthForComponents<PIX, maxValue, 3>(ptrs, r, g, b, a, bounds, roi, renderArgs);
            break;
        case 4:
            fillForDepthForComponents<PIX, maxValue, 4>(ptrs, r, g, b, a, bounds, roi, renderArgs);
            break;
        default:
            break;
    }
} // fillForDepth


void
ImagePrivate::fillCPU(void* ptrs[4],
                      float r,
                      float g,
                      float b,
                      float a,
                      int nComps,
                      ImageBitDepthEnum bitDepth,
                      const RectI& bounds,
                      const RectI& roi,
                      const TreeRenderNodeArgsPtr& renderArgs)
{
    if (r == 0 && g == 0 &&  b == 0 && a == 0) {
        fillCPUBlack(ptrs, nComps, bitDepth, bounds, roi, renderArgs);
        return;
    }

    switch (bitDepth) {
        case eImageBitDepthByte:
            fillForDepth<unsigned char, 255>(ptrs, r, g, b, a, nComps, bounds, roi, renderArgs);
            break;
        case eImageBitDepthFloat:
            fillForDepth<float, 1>(ptrs, r, g, b, a, nComps, bounds, roi, renderArgs);
            break;
        case eImageBitDepthShort:
            fillForDepth<unsigned short, 65535>(ptrs, r, g, b, a, nComps, bounds, roi, renderArgs);
            break;
        case eImageBitDepthHalf:
        default:
            break;
    }


} // fillCPU

NATRON_NAMESPACE_EXIT;
