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

#include "FrameEntry.h"

#include <cassert>
#include <cstring> // for std::memcpy, std::memset
#include <stdexcept>

#include "Engine/RectI.h"

NATRON_NAMESPACE_ENTER


const U8*
FrameEntry::pixelAt(int x,
                    int y ) const
{
    const TextureRect& bounds = _key.getTexRect();

    if ( (x < bounds.x1) || (x >= bounds.x2) || (y < bounds.y1) || (y > bounds.y2) ) {
        return 0;
    }
    std::size_t rowSize = bounds.width();
    unsigned int srcPixelSize = 4;
    if ( (ImageBitDepthEnum)_key.getBitDepth() == eImageBitDepthFloat ) {
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
    std::size_t srcRowSize = srcBounds.width();
    unsigned int srcPixelSize = 4;
    if ( (ImageBitDepthEnum)other.getKey().getBitDepth() == eImageBitDepthFloat ) {
        srcPixelSize *= sizeof(float);
    }
    srcRowSize *= srcPixelSize;

    std::size_t dstRowSize = srcBounds.width();
    unsigned int dstPixelSize = 4;
    if ( (ImageBitDepthEnum)_key.getBitDepth() == eImageBitDepthFloat ) {
        dstPixelSize *= sizeof(float);
    }
    dstRowSize *= dstPixelSize;

    // Fill with black and transparent because src might be smaller
    if ( !srcPixels ||
         !srcBounds.contains(dstBounds) ||
         other.getKey().getBitDepth() != _key.getBitDepth() ) {
        std::memset( dstPixels, 0, dstRowSize * dstBounds.height() );

        return;
    }

    // Copy pixels over the intersection
    RectI srcBoundsRect;
    srcBoundsRect.x1 = srcBounds.x1;
    srcBoundsRect.x2 = srcBounds.x2;
    srcBoundsRect.y1 = srcBounds.y1;
    srcBoundsRect.y2 = srcBounds.y2;
    RectI roi;
    if ( !dstBounds.intersect(srcBoundsRect, &roi) ) {
        return;
    }

    dstPixels += (roi.y1 - dstBounds.y1) * dstRowSize + (roi.x1 - dstBounds.x1) * dstPixelSize;
    srcPixels += (roi.y1 - srcBounds.y1) * srcRowSize + (roi.x1 - srcBounds.x1) * srcPixelSize;

    std::size_t roiRowSize = dstPixelSize * roi.width();

    //Align dstPixel to srcPixels point
    for (int y = roi.y1; y < roi.y2; ++y,
         srcPixels += srcRowSize,
         dstPixels += dstRowSize) {
        std::memcpy(dstPixels, srcPixels, roiRowSize);
    }
} // FrameEntry::copy

NATRON_NAMESPACE_EXIT
