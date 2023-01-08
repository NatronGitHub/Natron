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

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Image.h"

#include <cassert>
#include <stdexcept>
#include "Engine/GLShader.h"
#include "Engine/OSGLContext.h"

NATRON_NAMESPACE_ENTER

template<int srcNComps, int dstNComps, typename PIX, int maxValue, bool masked, bool maskInvert>
void
Image::applyMaskMixForMaskInvert(const RectI& roi,
                                 const Image* maskImg,
                                 const Image* originalImg,
                                 float mix)
{
    PIX* dst_pixels = (PIX*)pixelAt(roi.x1, roi.y1);
    unsigned int dstRowElements = _bounds.width() * getComponentsCount();

    for ( int y = roi.y1; y < roi.y2; ++y,
          dst_pixels += (dstRowElements - (roi.x2 - roi.x1) * dstNComps) ) { // 1 row stride minus what was done at previous iteration
        for (int x = roi.x1; x < roi.x2; ++x,
             dst_pixels += dstNComps) {
            const PIX* src_pixels = originalImg ? (const PIX*)originalImg->pixelAt(x, y) : 0;
            float maskScale = 1.f;
            if (!masked) {
                // just mix
                float alpha = mix;
                if (src_pixels) {
                    for (int c = 0; c < dstNComps; ++c) {
                        if (c < srcNComps) {
                            float v = float(dst_pixels[c]) * alpha + (1.f - alpha) * float(src_pixels[c]);
                            dst_pixels[c] = clampIfInt<PIX>(v);
                        }
                    }
                } else {
                    for (int c = 0; c < dstNComps; ++c) {
                        float v = float(dst_pixels[c]) * alpha;
                        dst_pixels[c] = clampIfInt<PIX>(v);
                    }
                }
            } else {
                const PIX* maskPixels = maskImg ? (const PIX*)maskImg->pixelAt(x, y) : 0;
                // figure the scale factor from that pixel
                if (maskPixels == 0) {
                    maskScale = maskInvert ? 1.f : 0.f;
                } else {
                    maskScale = *maskPixels * (1.f / maxValue);
                    if (maskInvert) {
                        maskScale = 1.f - maskScale;
                    }
                }
                float alpha = mix * maskScale;
                if (src_pixels) {
                    for (int c = 0; c < dstNComps; ++c) {
                        if (c < srcNComps) {
                            float v = float(dst_pixels[c]) * alpha + (1.f - alpha) * float(src_pixels[c]);
                            dst_pixels[c] = clampIfInt<PIX>(v);
                        }
                    }
                } else {
                    for (int c = 0; c < dstNComps; ++c) {
                        float v = float(dst_pixels[c]) * alpha;
                        dst_pixels[c] = clampIfInt<PIX>(v);
                    }
                }
            }
        }
    }
} // Image::applyMaskMixForMaskInvert

template<int srcNComps, int dstNComps, typename PIX, int maxValue, bool masked>
void
Image::applyMaskMixForMasked(const RectI& roi,
                             const Image* maskImg,
                             const Image* originalImg,
                             bool maskInvert,
                             float mix)
{
    if (maskInvert) {
        applyMaskMixForMaskInvert<srcNComps, dstNComps, PIX, maxValue, masked, true>(roi, maskImg, originalImg, mix);
    } else {
        applyMaskMixForMaskInvert<srcNComps, dstNComps, PIX, maxValue, masked, false>(roi, maskImg, originalImg, mix);
    }
}

template<int srcNComps, int dstNComps, typename PIX, int maxValue>
void
Image::applyMaskMixForDepth(const RectI& roi,
                            const Image* maskImg,
                            const Image* originalImg,
                            bool masked,
                            bool maskInvert,
                            float mix)
{
    if (masked) {
        applyMaskMixForMasked<srcNComps, dstNComps, PIX, maxValue, true>(roi, maskImg, originalImg, maskInvert, mix);
    } else {
        applyMaskMixForMasked<srcNComps, dstNComps, PIX, maxValue, false>(roi, maskImg, originalImg, maskInvert, mix);
    }
}

template<int srcNComps, int dstNComps>
void
Image::applyMaskMixForDstComponents(const RectI& roi,
                                    const Image* maskImg,
                                    const Image* originalImg,
                                    bool masked,
                                    bool maskInvert,
                                    float mix)
{
    ImageBitDepthEnum depth = getBitDepth();

    switch (depth) {
    case eImageBitDepthByte:
        applyMaskMixForDepth<srcNComps, dstNComps, unsigned char, 255>(roi, maskImg, originalImg, masked, maskInvert, mix);
        break;
    case eImageBitDepthShort:
        applyMaskMixForDepth<srcNComps, dstNComps, unsigned short, 65535>(roi, maskImg, originalImg, masked, maskInvert, mix);
        break;
    case eImageBitDepthFloat:
        applyMaskMixForDepth<srcNComps, dstNComps, float, 1>(roi, maskImg, originalImg, masked, maskInvert, mix);
        break;
    default:
        assert(false);
        break;
    }
}

template<int srcNComps>
void
Image::applyMaskMixForSrcComponents(const RectI& roi,
                                    const Image* maskImg,
                                    const Image* originalImg,
                                    bool masked,
                                    bool maskInvert,
                                    float mix)
{
    int dstNComps = getComponentsCount();

    assert(0 < dstNComps && dstNComps <= 4);
    switch (dstNComps) {
    //case 0:
    //    applyMaskMixForDstComponents<srcNComps,0>(roi, maskImg, originalImg, masked, maskInvert, mix);
    //    break;
    case 1:
        applyMaskMixForDstComponents<srcNComps, 1>(roi, maskImg, originalImg, masked, maskInvert, mix);
        break;
    case 2:
        applyMaskMixForDstComponents<srcNComps, 2>(roi, maskImg, originalImg, masked, maskInvert, mix);
        break;
    case 3:
        applyMaskMixForDstComponents<srcNComps, 3>(roi, maskImg, originalImg, masked, maskInvert, mix);
        break;
    case 4:
        applyMaskMixForDstComponents<srcNComps, 4>(roi, maskImg, originalImg, masked, maskInvert, mix);
        break;
    default:
        break;
    }
}

void
Image::applyMaskMix(const RectI& roi,
                    const Image* maskImg,
                    const Image* originalImg,
                    bool masked,
                    bool maskInvert,
                    float mix,
                    const OSGLContextPtr& glContext)
{
    ///!masked && mix == 1 has nothing to do
    if ( !masked && (mix == 1) ) {
        return;
    }

    QWriteLocker k(&_entryLock);
    std::unique_ptr<QReadLocker> originalLock;
    std::unique_ptr<QReadLocker> maskLock;
    if (originalImg) {
        originalLock.reset( new QReadLocker(&originalImg->_entryLock) );
    }
    if (maskImg) {
        maskLock.reset( new QReadLocker(&maskImg->_entryLock) );
    }
    RectI realRoI;
    roi.intersect(_bounds, &realRoI);

    assert( !originalImg || getBitDepth() == originalImg->getBitDepth() );
    assert( !masked || !maskImg || maskImg->getComponents() == ImagePlaneDesc::getAlphaComponents() );

    if (getStorageMode() == eStorageModeGLTex) {
        assert(glContext);
        assert(!originalImg || originalImg->getStorageMode() == eStorageModeGLTex);
        GLShaderPtr shader = glContext->getOrCreateMaskMixShader(maskImg != 0);
        assert(shader);
        GLuint fboID = glContext->getFBOId();

        glBindFramebuffer(GL_FRAMEBUFFER, fboID);
        int target = getGLTextureTarget();
        glEnable(target);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture( target, getGLTextureID() );

        glTexParameteri (target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri (target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glTexParameteri (target, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri (target, GL_TEXTURE_WRAP_T, GL_REPEAT);

        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, target, getGLTextureID(), 0 /*LoD*/);
        glCheckFramebufferError();

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(target, originalImg ? originalImg->getGLTextureID() : 0);

        glTexParameteri (target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri (target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glTexParameteri (target, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri (target, GL_TEXTURE_WRAP_T, GL_REPEAT);

        glActiveTexture(GL_TEXTURE2);
        glBindTexture(target, maskImg ? maskImg->getGLTextureID() : 0);

        glTexParameteri (target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri (target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glTexParameteri (target, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri (target, GL_TEXTURE_WRAP_T, GL_REPEAT);


        shader->bind();
        shader->setUniform("originalImageTex", 1);
        shader->setUniform("maskImageTex", 2);
        shader->setUniform("outputImageTex", 0);
        shader->setUniform("mixValue", mix);
        shader->setUniform("maskEnabled", maskImg ? 1 : 0);
        applyTextureMapping(_bounds, realRoI);
        shader->unbind();


        glBindTexture(target, 0);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(target, 0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(target, 0);
        glCheckError();

        return;
    }

    int srcNComps = originalImg ? (int)originalImg->getComponentsCount() : 0;
    //assert(0 < srcNComps && srcNComps <= 4);
    switch (srcNComps) {
    //case 0:
    //    applyMaskMixForSrcComponents<0>(realRoI, maskImg, originalImg, masked, maskInvert, mix);
    //    break;
    case 1:
        applyMaskMixForSrcComponents<1>(realRoI, maskImg, originalImg, masked, maskInvert, mix);
        break;
    case 2:
        applyMaskMixForSrcComponents<2>(realRoI, maskImg, originalImg, masked, maskInvert, mix);
        break;
    case 3:
        applyMaskMixForSrcComponents<3>(realRoI, maskImg, originalImg, masked, maskInvert, mix);
        break;
    case 4:
        applyMaskMixForSrcComponents<4>(realRoI, maskImg, originalImg, masked, maskInvert, mix);
        break;
    default:
        break;
    }
} // applyMaskMix

NATRON_NAMESPACE_EXIT
