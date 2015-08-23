//  Natron
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 * Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 * contact: immarespond at gmail dot com
 *
 */

// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>

#include "FrameEntry.h"

#include "Engine/RectI.h"

using namespace Natron;



boost::shared_ptr<FrameParams>
FrameEntry::makeParams(const RectI & rod,
                       int bitDepth,
                       int texW,
                       int texH,
                       const boost::shared_ptr<Natron::Image>& image)
{
    return boost::shared_ptr<FrameParams>( new FrameParams(rod, bitDepth, texW, texH, image) );
}

const U8*
FrameEntry::pixelAt(int x, int y ) const
{
    const TextureRect& bounds = _key.getTexRect();
    if (x < bounds.x1 || x >= bounds.x2 || y < bounds.y1 || y > bounds.y2) {
        return 0;
    }
    std::size_t rowSize = bounds.w;
    unsigned int srcPixelSize = 4;
    if ((Natron::ImageBitDepthEnum)_key.getBitDepth() == Natron::eImageBitDepthFloat) {
        srcPixelSize *= sizeof(float);
    }
    rowSize *= srcPixelSize;
    return data() +  (y - bounds.y1) * rowSize + (x - bounds.x1) * srcPixelSize;
}

void
FrameEntry::copy(const FrameEntry& other)
{
    U8* dstPixels = data();
    assert(dstPixels);
    if (!dstPixels) {
        return;
    }
    const U8* srcPixels = other.data();
    
    const TextureRect& srcBounds = other.getKey().getTexRect();
    const TextureRect& dstBounds = _key.getTexRect();
    
    std::size_t srcRowSize = srcBounds.w;
    unsigned int srcPixelSize = 4;
    if ((Natron::ImageBitDepthEnum)other.getKey().getBitDepth() == Natron::eImageBitDepthFloat) {
        srcPixelSize *= sizeof(float);
    }
    srcRowSize *= srcPixelSize;
    
    std::size_t dstRowSize = srcBounds.w ;
    unsigned int dstPixelSize = 4;
    if ((Natron::ImageBitDepthEnum)_key.getBitDepth() == Natron::eImageBitDepthFloat) {
        dstPixelSize *= sizeof(float);
    }
    dstRowSize *= dstPixelSize;
    
    //Fill with black and transparant because src might be smaller
    bool filledZero = false;
    if (!srcBounds.contains(dstBounds)) {
        memset(dstPixels, 0, dstRowSize * dstBounds.h);
        filledZero = true;
    }
    if (other.getKey().getBitDepth() != _key.getBitDepth()) {
        if (!filledZero) {
            memset(dstPixels, 0, dstRowSize * dstBounds.h);
        }
        return;
    }
    
    // Copy pixels over the intersection
    RectI srcBoundsRect;
    srcBoundsRect.x1 = srcBounds.x1;
    srcBoundsRect.x2 = srcBounds.x2;
    srcBoundsRect.y1 = srcBounds.y1;
    srcBoundsRect.y2 = srcBounds.y2;
    RectI roi;
    if (!dstBounds.intersect(srcBoundsRect, &roi)) {
        return;
    }
    
    dstPixels += (roi.y1 - dstBounds.y1) * dstRowSize + (roi.x1 - dstBounds.x1) * dstPixelSize;
    srcPixels += (roi.y1 - srcBounds.y1) * srcRowSize + (roi.x1 - srcBounds.x1) * srcPixelSize;
    
    std::size_t roiRowSize = dstPixelSize * roi.width();
    
    //Align dstPixel to srcPixels point
    for (int y = roi.y1; y < roi.y2; ++y,
         srcPixels += srcRowSize,
         dstPixels += dstRowSize) {
        memcpy(dstPixels, srcPixels, roiRowSize);
    }
    
}