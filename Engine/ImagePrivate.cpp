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

int
ImagePrivate::getNTilesPerLine() const
{
    if (tiles.empty()) {
        return 0;
    }
    int tileSizeX = tiles[0].tileBounds.width();
    return bounds.width() / tileSizeX;
}


void
ImagePrivate::insertTilesInCache()
{
    // The image must have cache enabled, otherwise don't call this function.
    assert(cachePolicy == Image::eCacheAccessModeWriteOnly ||
           cachePolicy == Image::eCacheAccessModeReadWrite);

    CachePtr cache = appPTR->getCache();

    for (std::size_t tile_i = 0; tile_i < tiles.size(); ++tile_i) {
        Image::Tile& tile = tiles[tile_i];
        assert(!tile.perChannelTile.empty());

        for (std::size_t c = 0; c < tile.perChannelTile.size(); ++c) {

            Image::MonoChannelTile& thisChannelTile = tile.perChannelTile[c];

            // If the tile is already cached, don't push it to the cache
            if (thisChannelTile.isCached) {
                continue;
            }

            // If the tile was not cached, we MUST have an entry locker, otherwise maybe
            // another thread already computed and pushed this entry in the cache.
            assert(thisChannelTile.entryLocker);

            // Insert in the cache
            cache->insert(thisChannelTile.buffer, thisChannelTile.entryLocker);
            
        }
        
    } // for each tile
} // insertTilesInCache

const Image::Tile*
ImagePrivate::getTile(int x, int y) const
{
    if (!bounds.contains(x, y)) {
        // Out of bounds
        return 0;
    }
    if (tiles.size() == 1) {
        // Single tiled image
        return &tiles[0];
    }

    int tileSizeX = tiles[0].tileBounds.width();
    int tileSizeY = tiles[0].tileBounds.height();

    // Tiles must be aligned
    assert(bounds.width() % tileSizeX == 0);
    assert(bounds.height() % tileSizeY == 0);

    int nTilesPerLine = bounds.width() / tileSizeX;
    int tileX = std::floor((double)(x - bounds.x1) / tileSizeX);
    int tileY = std::floor((double)(y - bounds.y1) / tileSizeY);

    int tile_i = tileY * nTilesPerLine + tileX;
    assert(tile_i >= 0 && tile_i < (int)tiles.size());
    return &tiles[tile_i];
    
} // getTile

RectI
ImagePrivate::getTilesCoordinates(const RectI& pixelCoordinates) const
{
    if (tiles.empty()) {
        return RectI();
    }
    int tileSizeX = tiles[0].tileBounds.width();
    int tileSizeY = tiles[0].tileBounds.height();

    // Round the pixel coords to the tile size
    RectI roundedPixelCoords;
    roundedPixelCoords.x1 = std::max(bounds.x1, (int)std::floor( ( (double)pixelCoordinates.x1 ) / tileSizeX ) * tileSizeX);
    roundedPixelCoords.y1 = std::max(bounds.y1, (int)std::floor( ( (double)pixelCoordinates.y1 ) / tileSizeY ) * tileSizeY);
    roundedPixelCoords.x2 = std::min(bounds.x2, (int)std::ceil( ( (double)pixelCoordinates.x2 ) / tileSizeX ) * tileSizeX);
    roundedPixelCoords.y2 = std::min(bounds.y2, (int)std::ceil( ( (double)pixelCoordinates.y2 ) / tileSizeY ) * tileSizeY);

    // Ensure the tiles are aligned
    assert(((roundedPixelCoords.x1 - bounds.x1) % tileSizeX) == 0);
    assert(((roundedPixelCoords.y1 - bounds.y1) / tileSizeY) == 0);
    assert(((roundedPixelCoords.x2 - bounds.x1) / tileSizeX) == 0);
    assert(((roundedPixelCoords.y2 - bounds.y1) / tileSizeY) == 0);

    RectI tilesRect;
    tilesRect.x1 = (roundedPixelCoords.x1 - bounds.x1) / tileSizeX;
    tilesRect.y1 = (roundedPixelCoords.y1 - bounds.y1) / tileSizeY;
    tilesRect.x2 = (roundedPixelCoords.x2 - bounds.x1) / tileSizeX;
    tilesRect.y2 = (roundedPixelCoords.y2 - bounds.y1) / tileSizeY;
    return tilesRect;
} // getTilesCoordinates


/**
 * @brief If copying pixels from fromImage to toImage cannot be copied directly, this function
 * returns a temporary image that is suitable to copy then to the toImage.
 **/
ImagePtr
ImagePrivate::checkIfCopyToTempImageIsNeeded(const Image& fromImage, const Image& toImage, const RectI& roi)
{
    // Copying from a tiled buffer is not trivial unless we are not tiled.
    // If both are tiled, convert the original image to a packed format first
    if (fromImage._imp->bufferFormat == Image::eImageBufferLayoutMonoChannelTiled && toImage._imp->bufferFormat == Image::eImageBufferLayoutMonoChannelTiled) {
        ImagePtr tmpImage;
        Image::InitStorageArgs args;
        args.bounds = roi;
        args.layer = fromImage._imp->layer;
        tmpImage = Image::create(args);

        Image::CopyPixelsArgs copyArgs;
        copyArgs.roi = roi;
        tmpImage->copyPixels(fromImage, copyArgs);
        return tmpImage;
    }

    // OpenGL textures may only be read from a RGBA packed buffer
    if (fromImage.getStorageMode() == eStorageModeGLTex) {

        // If this is also an OpenGL texture, check they have the same context otherwise first bring back
        // the image to CPU
        if (toImage.getStorageMode() == eStorageModeGLTex) {

            GLCacheEntryPtr isGlEntry = toGLCacheEntry(toImage._imp->tiles[0].perChannelTile[0].buffer);
            GLCacheEntryPtr otherIsGlEntry = toGLCacheEntry(fromImage._imp->tiles[0].perChannelTile[0].buffer);
            assert(isGlEntry && otherIsGlEntry);
            if (isGlEntry->getOpenGLContext() != otherIsGlEntry->getOpenGLContext()) {
                ImagePtr tmpImage;
                Image::InitStorageArgs args;
                args.bounds = fromImage.getBounds();
                args.layer = ImageComponents::getRGBAComponents();
                tmpImage = Image::create(args);

                Image::CopyPixelsArgs copyArgs;
                copyArgs.roi = roi;
                tmpImage->copyPixels(fromImage, copyArgs);
                return tmpImage;
            }
        }

        // Converting from OpenGL to CPU requires a RGBA buffer with the same bounds
        if (toImage._imp->bufferFormat != Image::eImageBufferLayoutRGBAPackedFullRect || toImage.getComponentsCount() != 4 || toImage.getBounds() != fromImage.getBounds()) {
            ImagePtr tmpImage;
            Image::InitStorageArgs args;
            args.bounds = fromImage.getBounds();
            args.layer = ImageComponents::getRGBAComponents();
            tmpImage = Image::create(args);

            Image::CopyPixelsArgs copyArgs;
            copyArgs.roi = roi;
            tmpImage->copyPixels(fromImage, copyArgs);
            return tmpImage;

        }

        // All other cases can copy fine
        return ImagePtr();
    }

    // OpenGL textures may only be written from a RGBA packed buffer
    if (toImage.getStorageMode() == eStorageModeGLTex) {

        // Converting to OpenGl requires an RGBA buffer
        if (fromImage._imp->bufferFormat != Image::eImageBufferLayoutRGBAPackedFullRect || fromImage.getComponentsCount() != 4) {
            ImagePtr tmpImage;
            Image::InitStorageArgs args;
            args.bounds = fromImage.getBounds();
            args.layer = ImageComponents::getRGBAComponents();
            tmpImage = Image::create(args);

            Image::CopyPixelsArgs copyArgs;
            copyArgs.roi = roi;
            tmpImage->copyPixels(fromImage, copyArgs);
            return tmpImage;
        }
    }

    // All other cases can copy fine
    return ImagePtr();
} // checkIfCopyToTempImageIsNeeded

void
ImagePrivate::copyUntiledImageToTiledImage(const Image& fromImage, const Image::CopyPixelsArgs& args)
{
    assert(bufferFormat == Image::eImageBufferLayoutMonoChannelTiled);
    assert(bounds.contains(args.roi) && fromImage._imp->bounds.contains(args.roi));

    // If this image is tiled, the other image must not be tiled
    assert(fromImage._imp->bufferFormat != Image::eImageBufferLayoutMonoChannelTiled);

    assert(fromImage._imp->tiles[0].perChannelTile[0].channelIndex == -1);

    const int nTilesPerLine = getNTilesPerLine();
    const RectI tilesRect = getTilesCoordinates(args.roi);

    const StorageModeEnum fromStorage = fromImage.getStorageMode();
    const StorageModeEnum toStorage = tiles[0].perChannelTile[0].buffer->getStorageMode();

    Image::CopyPixelsArgs argsCpy = args;

    // Copy each tile individually
    for (int ty = tilesRect.y1; ty < tilesRect.y2; ++ty) {
        for (int tx = tilesRect.x1; tx < tilesRect.x2; ++tx) {
            int tile_i = tx + ty * nTilesPerLine;
            assert(tile_i >= 0 && tile_i < (int)tiles.size());

            // This is the tile to write to
            const Image::Tile& thisTile = tiles[tile_i];

            thisTile.tileBounds.intersect(args.roi, &argsCpy.roi);

            ImagePrivate::copyRectangle(fromImage._imp->tiles[0], fromStorage, fromImage._imp->bufferFormat, thisTile, toStorage, bufferFormat, argsCpy);

        } // for all tiles horizontally
    } // for all tiles vertically
    
} // copyUntiledImageToTiledImage

void
ImagePrivate::copyTiledImageToUntiledImage(const Image& fromImage, const Image::CopyPixelsArgs& args)
{
    // The input image may or may not be tiled, but we surely are not.
    assert(bufferFormat != Image::eImageBufferLayoutMonoChannelTiled);
    assert(bounds.contains(args.roi) && fromImage._imp->bounds.contains(args.roi));
    assert(tiles[0].perChannelTile.size() == 1 && tiles[0].perChannelTile[0].channelIndex == -1);
    assert(tiles[0].perChannelTile[0].channelIndex == -1);

    const int nTilesPerLine = fromImage._imp->getNTilesPerLine();
    const RectI tilesRect = fromImage._imp->getTilesCoordinates(args.roi);

    const StorageModeEnum fromStorage = fromImage.getStorageMode();
    const StorageModeEnum toStorage = tiles[0].perChannelTile[0].buffer->getStorageMode();
    Image::CopyPixelsArgs argsCpy = args;

    // Copy each tile individually
    for (int ty = tilesRect.y1; ty < tilesRect.y2; ++ty) {
        for (int tx = tilesRect.x1; tx < tilesRect.x2; ++tx) {
            int tile_i = tx + ty * nTilesPerLine;
            assert(tile_i >= 0 && tile_i < (int)fromImage._imp->tiles.size());

            // This is the tile to write to
            const Image::Tile& fromTile = fromImage._imp->tiles[tile_i];

            fromTile.tileBounds.intersect(args.roi, &argsCpy.roi);

            ImagePrivate::copyRectangle(fromTile, fromStorage, fromImage._imp->bufferFormat, tiles[0], toStorage, bufferFormat, argsCpy);

        } // for all tiles horizontally
    } // for all tiles vertically


} // copyTiledImageToUntiledImage

void
ImagePrivate::copyUntiledImageToUntiledImage(const Image& fromImage, const Image::CopyPixelsArgs& args)
{
    // The input image may or may not be tiled, but we surely are not.
    assert(bufferFormat != Image::eImageBufferLayoutMonoChannelTiled);
    assert(bounds.contains(args.roi) && fromImage._imp->bounds.contains(args.roi));
    assert(fromImage._imp->tiles.size() == 1 && tiles.size() == 1);
    assert(tiles[0].perChannelTile.size() == 1 && tiles[0].perChannelTile[0].channelIndex == -1);
    assert(fromImage._imp->tiles[0].perChannelTile.size() == 1 && fromImage._imp->tiles[0].perChannelTile[0].channelIndex == -1);

    const StorageModeEnum fromStorage = fromImage.getStorageMode();
    const StorageModeEnum toStorage = tiles[0].perChannelTile[0].buffer->getStorageMode();

    ImagePrivate::copyRectangle(fromImage._imp->tiles[0], fromStorage, fromImage._imp->bufferFormat, tiles[0], toStorage, bufferFormat, args);

} // copyUntiledImageToUntiledImage

template <typename PIX, int maxValue, int nComps>
static void
halveImageForInternal(const void* srcPtrs[4],
                      const RectI& srcBounds,
                      void* dstPtrs[4],
                      const RectI& dstBounds)
{

    PIX* dstPixelPtrs[4];
    int dstPixelStride;
    Image::getChannelPointers<PIX, nComps>((PIX**)dstPtrs, dstBounds.x1, dstBounds.y1, dstBounds, dstPixelPtrs, &dstPixelStride);

    const PIX* srcPixelPtrs[4];
    int srcPixelStride;
    Image::getChannelPointers<PIX, nComps>((PIX**)srcPtrs, srcBounds.x1, srcBounds.y1, srcBounds, srcPixelPtrs, &srcPixelStride);

    const int dstRowElementsCount = dstBounds.width() * dstPixelStride;
    const int srcRowElementsCount = srcBounds.width() * srcPixelStride;


    for (int y = 0; y < dstBounds.height(); ++y) {

        // The current dst row, at y, covers the src rows y*2 (thisRow) and y*2+1 (nextRow).
        const int srcy = y * 2;

        // Check that we are within srcBounds.
        const bool pickThisRow = srcBounds.y1 <= (srcy + 0) && (srcy + 0) < srcBounds.y2;
        const bool pickNextRow = srcBounds.y1 <= (srcy + 1) && (srcy + 1) < srcBounds.y2;

        const int sumH = (int)pickNextRow + (int)pickThisRow;
        assert(sumH == 1 || sumH == 2);

        for (int x = 0; x < dstBounds.width(); ++x) {

            // The current dst col, at y, covers the src cols x*2 (thisCol) and x*2+1 (nextCol).
            const int srcx = x * 2;

            // Check that we are within srcBounds.
            const bool pickThisCol = srcBounds.x1 <= (srcx + 0) && (srcx + 0) < srcBounds.x2;
            const bool pickNextCol = srcBounds.x1 <= (srcx + 1) && (srcx + 1) < srcBounds.x2;

            const int sumW = (int)pickThisCol + (int)pickNextCol;
            assert(sumW == 1 || sumW == 2);

            const int sum = sumW * sumH;
            assert(0 < sum && sum <= 4);

            for (int k = 0; k < nComps; ++k) {

                // Averaged pixels are as such:
                // a b
                // c d

                const PIX a = (pickThisCol && pickThisRow) ? *(srcPixelPtrs[k]) : 0;
                const PIX b = (pickNextCol && pickThisRow) ? *(srcPixelPtrs[k] + srcPixelStride) : 0;
                const PIX c = (pickThisCol && pickNextRow) ? *(srcPixelPtrs[k] + srcRowElementsCount) : 0;
                const PIX d = (pickNextCol && pickNextRow) ? *(srcPixelPtrs[k] + srcRowElementsCount + srcPixelStride)  : 0;

                assert( sumW == 2 || ( sumW == 1 && ( (a == 0 && c == 0) || (b == 0 && d == 0) ) ) );
                assert( sumH == 2 || ( sumH == 1 && ( (a == 0 && b == 0) || (c == 0 && d == 0) ) ) );

                *dstPixelPtrs[k] = (a + b + c + d) / sum;

                srcPixelPtrs[k] += srcPixelStride * 2;
                dstPixelPtrs[k] += dstPixelStride;
            } // for each component
            
        } // for each pixels on the line

        // Remove what was offset to the pointers during this scan-line and offset to the next
        for (int k = 0; k < nComps; ++k) {
            dstPixelPtrs[k] += (dstRowElementsCount - dstBounds.width() * dstPixelStride);
            srcPixelPtrs[k] += (srcRowElementsCount * 2 - dstBounds.width() * srcPixelStride);
        }
    }  // for each scan line
} // halveImageForInternal


template <typename PIX, int maxValue>
static void
halveImageForDepth(const void* srcPtrs[4],
                   int nComps,
                   const RectI& srcBounds,
                   void* dstPtrs[4],
                   const RectI& dstBounds)
{
    switch (nComps) {
        case 1:
            halveImageForInternal<PIX, maxValue, 1>(srcPtrs, srcBounds, dstPtrs, dstBounds);
            break;
        case 2:
            halveImageForInternal<PIX, maxValue, 2>(srcPtrs, srcBounds, dstPtrs, dstBounds);
            break;
        case 3:
            halveImageForInternal<PIX, maxValue, 3>(srcPtrs, srcBounds, dstPtrs, dstBounds);
            break;
        case 4:
            halveImageForInternal<PIX, maxValue, 4>(srcPtrs, srcBounds, dstPtrs, dstBounds);
            break;
        default:
            break;
    }
}


void
ImagePrivate::halveImage(const void* srcPtrs[4],
                         int nComps,
                         ImageBitDepthEnum bitDepth,
                         const RectI& srcBounds,
                         void* dstPtrs[4],
                         const RectI& dstBounds)
{
    switch ( bitDepth ) {
        case eImageBitDepthByte:
            halveImageForDepth<unsigned char, 255>(srcPtrs, nComps, srcBounds, dstPtrs, dstBounds);
            break;
        case eImageBitDepthShort:
            halveImageForDepth<unsigned short, 65535>(srcPtrs, nComps, srcBounds, dstPtrs, dstBounds);
            break;
        case eImageBitDepthHalf:
            assert(false);
            break;
        case eImageBitDepthFloat:
            halveImageForDepth<float, 1>(srcPtrs, nComps, srcBounds, dstPtrs, dstBounds);
            break;
        case eImageBitDepthNone:
            break;
    }
} // halveImage

template <typename PIX, int maxValue, int nComps>
bool
checkForNaNsInternal(void* ptrs[4],
                     const RectI& bounds,
                     const RectI& roi)
{

    PIX* dstPixelPtrs[4];
    int dstPixelStride;
    Image::getChannelPointers<PIX, nComps>((PIX**)ptrs, roi.x1, roi.y1, bounds, dstPixelPtrs, &dstPixelStride);
    const int rowElementsCount = bounds.width() * dstPixelStride;

    bool hasnan = false;
    for (int y = roi.y1; y < roi.y2; ++y) {
        for (int x = roi.x1; x < roi.x2; ++x) {
            for (int k = 0; k < nComps; ++k) {
                // we remove NaNs, but infinity values should pose no problem
                // (if they do, please explain here which ones)
                if (*dstPixelPtrs[k] != *dstPixelPtrs[k]) { // check for NaN
                    *dstPixelPtrs[k] = 1.;
                    ++dstPixelPtrs[k];
                    hasnan = true;
                }
            }
        }
        // Remove what was done at the previous scan-line and got to the next
        for (int k = 0; k < nComps; ++k) {
            dstPixelPtrs[k] += (rowElementsCount - roi.width() * dstPixelStride);
        }
    } // for each scan-line

    return hasnan;
} // checkForNaNsInternal

template <typename PIX, int maxValue>
bool
checkForNaNsForDepth(void* ptrs[4],
                     int nComps,
                     const RectI& bounds,
                     const RectI& roi)
{
    switch (nComps) {
        case 1:
            return checkForNaNsInternal<PIX, maxValue, 1>(ptrs, bounds, roi);
            break;
        case 2:
            return checkForNaNsInternal<PIX, maxValue, 2>(ptrs, bounds, roi);
            break;
        case 3:
            return checkForNaNsInternal<PIX, maxValue, 3>(ptrs, bounds, roi);
            break;
        case 4:
            return checkForNaNsInternal<PIX, maxValue, 4>(ptrs, bounds, roi);
            break;
        default:
            break;
    }
    return false;

}


bool
ImagePrivate::checkForNaNs(void* ptrs[4],
                           int nComps,
                           ImageBitDepthEnum bitdepth,
                           const RectI& bounds,
                           const RectI& roi)
{
    switch ( bitdepth ) {
        case eImageBitDepthByte:
            return checkForNaNsForDepth<unsigned char, 255>(ptrs, nComps, bounds, roi);
            break;
        case eImageBitDepthShort:
            return checkForNaNsForDepth<unsigned short, 65535>(ptrs, nComps, bounds, roi);
            break;
        case eImageBitDepthHalf:
            assert(false);
            break;
        case eImageBitDepthFloat:
            return checkForNaNsForDepth<float, 1>(ptrs, nComps, bounds, roi);
            break;
        case eImageBitDepthNone:
            break;
    }
    return false;
}

NATRON_NAMESPACE_EXIT;