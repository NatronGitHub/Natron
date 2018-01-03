/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
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

template<int srcNComps, int dstNComps, typename PIX, int maxValue, bool masked, bool maskInvert>
static ActionRetCodeEnum
applyMaskMixForMaskInvert(const void* originalImgPtrs[4],
                          const RectI& originalImgBounds,
                          const void* maskImgPtrs[4],
                          const RectI& maskImgBounds,
                          void* dstImgPtrs[4],
                          double mix,
                          const RectI& bounds,
                          const RectI& roi,
                          const EffectInstancePtr& renderClone)
{

    PIX* dstPixelPtrs[4] = {NULL, NULL, NULL, NULL};
    int dstPixelStride;
    Image::getChannelPointers<PIX, dstNComps>((const PIX**)dstImgPtrs, roi.x1, roi.y1, bounds, (PIX**)dstPixelPtrs, &dstPixelStride);

    const std::size_t dstRowElements = (std::size_t)bounds.width() * dstPixelStride;

    for ( int y = roi.y1; y < roi.y2; ++y) {

        if (renderClone && renderClone->isRenderAborted()) {
            return eActionStatusAborted;
        }
        for (int x = roi.x1; x < roi.x2; ++x) {

            PIX* srcPixelPtrs[4] = {NULL, NULL, NULL, NULL};
            int srcPixelStride = 0;
            if (originalImgPtrs[0]) {
                Image::getChannelPointers<PIX, srcNComps>((const PIX**)originalImgPtrs, x, y, originalImgBounds, (PIX**)srcPixelPtrs, &srcPixelStride);
            }

            float maskScale = 1.f;
            if (!masked) {
                // just mix
                float alpha = mix;
                for (int c = 0; c < dstNComps; ++c) {
                    float dstF = Image::convertPixelDepth<PIX, float>(*dstPixelPtrs[c]);
                    float srcF;
                    if (*srcPixelPtrs[c]) {
                        srcF = Image::convertPixelDepth<PIX, float>(*srcPixelPtrs[c]);
                        srcPixelPtrs[c] += srcPixelStride;
                    } else {
                        srcF = 0.;
                    }
                    float v = dstF * alpha + (1.f - alpha) * srcF;
                    *dstPixelPtrs[c] = Image::convertPixelDepth<float, PIX>(v);

                    dstPixelPtrs[c] += dstPixelStride;
                }
            } else {

                PIX* maskPixelPtrs[4] = {NULL, NULL, NULL, NULL};
                int maskPixelStride;
                Image::getChannelPointers<PIX, 1>((const PIX**)maskImgPtrs, x, y, maskImgBounds, (PIX**)maskPixelPtrs, &maskPixelStride);

                // figure the scale factor from that pixel
                if (!maskPixelPtrs[0]) {
                    maskScale = maskInvert ? 1.f : 0.f;
                } else {
                    maskScale = *maskPixelPtrs[0] * (1.f / maxValue);
                    if (maskInvert) {
                        maskScale = 1.f - maskScale;
                    }
                    maskPixelPtrs[0] += maskPixelStride;

                }
                float alpha = mix * maskScale;
                for (int c = 0; c < dstNComps; ++c) {
                    float dstF = Image::convertPixelDepth<PIX, float>(*dstPixelPtrs[c]);
                    float srcF;
                    if (*srcPixelPtrs[c]) {
                        srcF = Image::convertPixelDepth<PIX, float>(*srcPixelPtrs[c]);
                        srcPixelPtrs[c] += srcPixelStride;
                    } else {
                        srcF = 0.;
                    }
                    float v = dstF * alpha + (1.f - alpha) * srcF;
                    *dstPixelPtrs[c] = Image::convertPixelDepth<float, PIX>(v);
                    dstPixelPtrs[c] += dstPixelStride;
                }
            }
        }
        for (int c = 0; c < dstNComps; ++c) {
            dstPixelPtrs[c] += (dstRowElements - roi.width() * dstPixelStride);
        }
    }
    return eActionStatusOK;
} // applyMaskMixForMaskInvert

template<int srcNComps, int dstNComps, typename PIX, int maxValue, bool masked>
static ActionRetCodeEnum
applyMaskMixForMasked(const void* originalImgPtrs[4],
                      const RectI& originalImgBounds,
                      const void* maskImgPtrs[4],
                      const RectI& maskImgBounds,
                      void* dstImgPtrs[4],
                      double mix,
                      bool invertMask,
                      const RectI& bounds,
                      const RectI& roi,
                      const EffectInstancePtr& renderClone)
{
    if (invertMask) {
        return applyMaskMixForMaskInvert<srcNComps, dstNComps, PIX, maxValue, masked, true>(originalImgPtrs, originalImgBounds, maskImgPtrs, maskImgBounds, dstImgPtrs, mix, bounds, roi, renderClone);
    } else {
        return applyMaskMixForMaskInvert<srcNComps, dstNComps, PIX, maxValue, masked, false>(originalImgPtrs, originalImgBounds, maskImgPtrs, maskImgBounds, dstImgPtrs, mix, bounds, roi, renderClone);
    }
}

template<int srcNComps, int dstNComps, typename PIX, int maxValue>
static ActionRetCodeEnum
applyMaskMixForDepth(const void* originalImgPtrs[4],
                     const RectI& originalImgBounds,
                     const void* maskImgPtrs[4],
                     const RectI& maskImgBounds,
                     void* dstImgPtrs[4],
                     double mix,
                     bool invertMask,
                     const RectI& bounds,
                     const RectI& roi,
                     const EffectInstancePtr& renderClone)
{
    if (maskImgPtrs[0]) {
        return applyMaskMixForMasked<srcNComps, dstNComps, PIX, maxValue, true>(originalImgPtrs, originalImgBounds, maskImgPtrs, maskImgBounds, dstImgPtrs, mix, invertMask, bounds, roi, renderClone);
    } else {
        return applyMaskMixForMasked<srcNComps, dstNComps, PIX, maxValue, false>(originalImgPtrs, originalImgBounds, maskImgPtrs, maskImgBounds, dstImgPtrs, mix, invertMask, bounds, roi, renderClone);
    }
}

template <int srcNComps, int dstNComps>
static ActionRetCodeEnum
applyMaskMixForDstComponents(const void* originalImgPtrs[4],
                             const RectI& originalImgBounds,
                             const void* maskImgPtrs[4],
                             const RectI& maskImgBounds,
                             void* dstImgPtrs[4],
                             ImageBitDepthEnum dstImgBitDepth,
                             double mix,
                             bool invertMask,
                             const RectI& bounds,
                             const RectI& roi,
                             const EffectInstancePtr& renderClone)
{
    switch (dstImgBitDepth) {
        case eImageBitDepthByte:
            return applyMaskMixForDepth<srcNComps, dstNComps, unsigned char, 255>(originalImgPtrs, originalImgBounds, maskImgPtrs, maskImgBounds, dstImgPtrs, mix, invertMask, bounds, roi, renderClone);
        case eImageBitDepthShort:
            return applyMaskMixForDepth<srcNComps, dstNComps, unsigned short, 65535>(originalImgPtrs, originalImgBounds, maskImgPtrs, maskImgBounds, dstImgPtrs, mix, invertMask, bounds, roi, renderClone);
        case eImageBitDepthFloat:
            return applyMaskMixForDepth<srcNComps, dstNComps, float, 1>(originalImgPtrs, originalImgBounds, maskImgPtrs, maskImgBounds, dstImgPtrs, mix, invertMask, bounds, roi, renderClone);
        default:
            assert(false);
            return eActionStatusFailed;
    }
}

template<int srcNComps>
static ActionRetCodeEnum
applyMaskMixForSrcComponents(const void* originalImgPtrs[4],
                             const RectI& originalImgBounds,
                             const void* maskImgPtrs[4],
                             const RectI& maskImgBounds,
                             void* dstImgPtrs[4],
                             ImageBitDepthEnum dstImgBitDepth,
                             int dstImgNComps,
                             double mix,
                             bool invertMask,
                             const RectI& bounds,
                             const RectI& roi,
                             const EffectInstancePtr& renderClone)
{

    switch (dstImgNComps) {
        case 1:
            return applyMaskMixForDstComponents<srcNComps, 1>(originalImgPtrs, originalImgBounds, maskImgPtrs, maskImgBounds, dstImgPtrs, dstImgBitDepth, mix, invertMask, bounds, roi, renderClone);
        case 2:
            return applyMaskMixForDstComponents<srcNComps, 2>(originalImgPtrs, originalImgBounds, maskImgPtrs, maskImgBounds, dstImgPtrs, dstImgBitDepth, mix, invertMask, bounds, roi, renderClone);
        case 3:
            return applyMaskMixForDstComponents<srcNComps, 3>(originalImgPtrs, originalImgBounds, maskImgPtrs, maskImgBounds, dstImgPtrs, dstImgBitDepth, mix, invertMask, bounds, roi, renderClone);
        case 4:
            return applyMaskMixForDstComponents<srcNComps, 4>(originalImgPtrs, originalImgBounds, maskImgPtrs, maskImgBounds, dstImgPtrs, dstImgBitDepth, mix, invertMask, bounds, roi, renderClone);
        default:
            return eActionStatusFailed;
    }
}

ActionRetCodeEnum
ImagePrivate::applyMaskMixCPU(const void* originalImgPtrs[4],
                              const RectI& originalImgBounds,
                              int originalImgNComps,
                              const void* maskImgPtrs[4],
                              const RectI& maskImgBounds,
                              void* dstImgPtrs[4],
                              ImageBitDepthEnum dstImgBitDepth,
                              int dstImgNComps,
                              double mix,
                              bool invertMask,
                              const RectI& bounds,
                              const RectI& roi,
                              const EffectInstancePtr& renderClone)
{
    switch (originalImgNComps) {
        case 1:
            return applyMaskMixForSrcComponents<1>(originalImgPtrs, originalImgBounds, maskImgPtrs, maskImgBounds, dstImgPtrs, dstImgBitDepth, dstImgNComps, mix, invertMask, bounds, roi, renderClone);
        case 2:
            return applyMaskMixForSrcComponents<2>(originalImgPtrs, originalImgBounds, maskImgPtrs, maskImgBounds, dstImgPtrs, dstImgBitDepth, dstImgNComps, mix, invertMask, bounds, roi, renderClone);
        case 3:
            return applyMaskMixForSrcComponents<3>(originalImgPtrs, originalImgBounds, maskImgPtrs, maskImgBounds, dstImgPtrs, dstImgBitDepth, dstImgNComps, mix, invertMask, bounds, roi, renderClone);
        case 4:
            return applyMaskMixForSrcComponents<4>(originalImgPtrs, originalImgBounds, maskImgPtrs, maskImgBounds, dstImgPtrs, dstImgBitDepth, dstImgNComps, mix, invertMask, bounds, roi, renderClone);
        default:
            return applyMaskMixForSrcComponents<0>(originalImgPtrs, originalImgBounds, maskImgPtrs, maskImgBounds, dstImgPtrs, dstImgBitDepth, dstImgNComps, mix, invertMask, bounds, roi, renderClone);
    }

}

template <typename GL>
void applyMaskMixGLInternal(const GLImageStoragePtr& originalTexture,
                            const GLImageStoragePtr& maskTexture,
                            const GLImageStoragePtr& dstTexture,
                            double mix,
                            bool maskInvert,
                            const RectI& roi,
                            const OSGLContextPtr& glContext)
{
    GLShaderBasePtr shader = glContext->getOrCreateMaskMixShader(maskTexture.get() != 0, maskInvert);
    assert(shader);
    GLuint fboID = glContext->getOrCreateFBOId();


    int target = dstTexture->getGLTextureTarget();
    int dstTexID = dstTexture->getGLTextureID();
    int originalTexID = originalTexture ? originalTexture->getGLTextureID() : 0;
    int maskTexID = maskTexture ? maskTexture->getGLTextureID() : 0;

    GL::BindFramebuffer(GL_FRAMEBUFFER, fboID);
    GL::Enable(target);
    GL::ActiveTexture(GL_TEXTURE0);
    GL::BindTexture( target, dstTexID );

    GL::TexParameteri (target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    GL::TexParameteri (target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    GL::TexParameteri (target, GL_TEXTURE_WRAP_S, GL_REPEAT);
    GL::TexParameteri (target, GL_TEXTURE_WRAP_T, GL_REPEAT);

    GL::FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, target, dstTexID, 0 /*LoD*/);
    glCheckFramebufferError(GL);

    GL::ActiveTexture(GL_TEXTURE1);
    GL::BindTexture(target, originalTexID);

    GL::TexParameteri (target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    GL::TexParameteri (target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    GL::TexParameteri (target, GL_TEXTURE_WRAP_S, GL_REPEAT);
    GL::TexParameteri (target, GL_TEXTURE_WRAP_T, GL_REPEAT);

    GL::ActiveTexture(GL_TEXTURE2);
    GL::BindTexture(target, maskTexID);

    GL::TexParameteri (target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    GL::TexParameteri (target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    GL::TexParameteri (target, GL_TEXTURE_WRAP_S, GL_REPEAT);
    GL::TexParameteri (target, GL_TEXTURE_WRAP_T, GL_REPEAT);

    const RectI& dstBounds = dstTexture->getBounds();
    const RectI& srcBounds = originalTexture ? originalTexture->getBounds() : dstBounds;

    shader->bind();
    shader->setUniform("originalImageTex", 1);
    shader->setUniform("maskImageTex", 2);
    shader->setUniform("outputImageTex", 0);
    shader->setUniform("mixValue", (float)mix);
    shader->setUniform("maskEnabled", (maskTexture.get()) ? 1 : 0);
    OSGLContext::applyTextureMapping<GL>(srcBounds, dstBounds, roi);
    shader->unbind();


    GL::BindTexture(target, 0);
    GL::ActiveTexture(GL_TEXTURE1);
    GL::BindTexture(target, 0);
    GL::ActiveTexture(GL_TEXTURE0);
    GL::BindTexture(target, 0);
    glCheckError(GL);

} // applyMaskMixGLInternal

void
ImagePrivate::applyMaskMixGL(const GLImageStoragePtr& originalTexture,
                             const GLImageStoragePtr& maskTexture,
                             const GLImageStoragePtr& dstTexture,
                             double mix,
                             bool invertMask,
                             const RectI& roi)
{
    OSGLContextPtr context = dstTexture->getOpenGLContext();
    assert(context);

    // Save the current context
    OSGLContextSaver saveCurrentContext;

    {
        // Ensure this context is attached
        OSGLContextAttacherPtr contextAttacher = OSGLContextAttacher::create(context);
        contextAttacher->attach();

        if (context->isGPUContext()) {
            applyMaskMixGLInternal<GL_GPU>(originalTexture, maskTexture, dstTexture, mix, invertMask, roi, context);
        } else {
            applyMaskMixGLInternal<GL_CPU>(originalTexture, maskTexture, dstTexture, mix, invertMask, roi, context);
        }
    }
}

NATRON_NAMESPACE_EXIT
