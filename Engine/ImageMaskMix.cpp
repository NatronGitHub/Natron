//  Natron
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>

#include "Image.h"

using namespace Natron;

template<int srcNComps, int dstNComps, typename PIX, int maxValue, bool masked, bool maskInvert>
void
Image::applyMaskMixForMaskInvert(const RectI& roi,
                                 const Image* maskImg,
                                 const Image* originalImg,
                                 float mix)
{
    PIX* dst_pixels = (PIX*)pixelAt(roi.x1, roi.y1);
    
    unsigned int dstRowElements = _bounds.width() * getComponentsCount();
    
    for (int y = roi.y1; y < roi.y2; ++y,
         dst_pixels += (dstRowElements - (roi.x2 - roi.x1) * dstNComps)) { // 1 row stride minus what was done at previous iteration
        
        for (int x = roi.x1; x < roi.x2; ++x,
             dst_pixels += dstNComps) {
            
            const PIX* src_pixels = originalImg ? (const PIX*)originalImg->pixelAt(x,y) : 0;
            
            float maskScale = 1.f;
            if (!masked) {
                // just mix
                float alpha = mix;
                if (src_pixels) {
                    for (int c = 0; c < dstNComps; ++c) {
                        if (c < srcNComps) {
                            float v = float(dst_pixels[c]) * alpha + (1.f - alpha) * float(src_pixels[c]);
                            dst_pixels[c] = (PIX)clampIfInt(v);
                        }
                        
                    }
                } else {
                    for (int c = 0; c < dstNComps; ++c) {
                        float v = float(dst_pixels[c]) * alpha;
                        dst_pixels[c] = (PIX)clampIfInt(v);
                    }
                }
                
            } else {
                const PIX* maskPixels = maskImg ? (const PIX*)maskImg->pixelAt(x,y) : 0;
                // figure the scale factor from that pixel
                if (maskPixels == 0) {
                    maskScale = maskInvert ? 1.f : 0.f;
                } else {
                    maskScale = *maskPixels / float(maxValue);
                    if (maskInvert) {
                        maskScale = 1.f - maskScale;
                    }
                }
                float alpha = mix * maskScale;
                if (src_pixels) {
                    for (int c = 0; c < dstNComps; ++c) {
                        if (c < srcNComps) {
                            float v = float(dst_pixels[c]) * alpha + (1.f - alpha) * float(src_pixels[c]);
                            dst_pixels[c] = (PIX)clampIfInt(v);
                        }
                        
                    }
                } else {
                    for (int c = 0; c < dstNComps; ++c) {
                        float v = float(dst_pixels[c]) * alpha;
                        dst_pixels[c] = (PIX)clampIfInt(v);
                    }
                }
            }
            
            
            
        }
    }
}


template<int srcNComps, int dstNComps, typename PIX, int maxValue, bool masked>
void
Image::applyMaskMixForMasked(const RectI& roi,
                             const Image* maskImg,
                             const Image* originalImg,
                             bool maskInvert,
                             float mix)
{
    if (maskInvert) {
        applyMaskMixForMaskInvert<srcNComps,dstNComps, PIX, maxValue, masked, true>(roi, maskImg, originalImg, mix);
    } else {
        applyMaskMixForMaskInvert<srcNComps,dstNComps, PIX, maxValue, masked, false>(roi, maskImg, originalImg, mix);
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
        applyMaskMixForMasked<srcNComps,dstNComps, PIX, maxValue, true>(roi, maskImg, originalImg, maskInvert, mix);
    } else {
        applyMaskMixForMasked<srcNComps,dstNComps, PIX, maxValue, false>(roi, maskImg, originalImg, maskInvert, mix);
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
    Natron::ImageBitDepthEnum depth = getBitDepth();
    switch (depth) {
        case eImageBitDepthByte:
            applyMaskMixForDepth<srcNComps,dstNComps, unsigned char , 255>(roi, maskImg, originalImg, masked, maskInvert, mix);
            break;
        case eImageBitDepthShort:
            applyMaskMixForDepth<srcNComps,dstNComps, unsigned short , 65535>(roi, maskImg, originalImg, masked, maskInvert, mix);
            break;
        case eImageBitDepthFloat:
            applyMaskMixForDepth<srcNComps,dstNComps, float, 1>(roi, maskImg, originalImg, masked, maskInvert, mix);
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
    switch (dstNComps) {
        case 0:
            applyMaskMixForDstComponents<srcNComps,0>(roi, maskImg, originalImg, masked, maskInvert, mix);
            break;
        case 1:
            applyMaskMixForDstComponents<srcNComps,1>(roi, maskImg, originalImg, masked, maskInvert, mix);
            break;
        case 2:
            applyMaskMixForDstComponents<srcNComps,2>(roi, maskImg, originalImg, masked, maskInvert, mix);
            break;
        case 3:
            applyMaskMixForDstComponents<srcNComps,3>(roi, maskImg, originalImg, masked, maskInvert, mix);
            break;
        case 4:
            applyMaskMixForDstComponents<srcNComps,4>(roi, maskImg, originalImg, masked, maskInvert, mix);
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
                    float mix)
{
    ///!masked && mix == 1 has nothing to do
    if (!masked && mix == 1) {
        return;
    }
    
    QWriteLocker k(&_entryLock);
    boost::shared_ptr<QReadLocker> originalLock;
    boost::shared_ptr<QReadLocker> maskLock;
    if (originalImg) {
        originalLock.reset(new QReadLocker(&originalImg->_entryLock));
    }
    if (maskImg) {
        maskLock.reset(new QReadLocker(&maskImg->_entryLock));
    }
    RectI realRoI;
    roi.intersect(_bounds, &realRoI);
    
    assert(!originalImg || getBitDepth() == originalImg->getBitDepth());
    assert(!masked || !maskImg || maskImg->getComponents() == ImageComponents::getAlphaComponents());
    
    int srcNComps = originalImg ? (int)originalImg->getComponentsCount() : 0;
    switch (srcNComps) {
        case 0:
            applyMaskMixForSrcComponents<0>(realRoI, maskImg, originalImg, masked, maskInvert, mix);
            break;
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
    
}
