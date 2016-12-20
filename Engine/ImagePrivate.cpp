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
        ImageTile& tile = tiles[tile_i];
        assert(!tile.perChannelTile.empty());

        for (std::size_t c = 0; c < tile.perChannelTile.size(); ++c) {

            MonoChannelTile& thisChannelTile = tile.perChannelTile[c];

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

const ImageTile*
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




NATRON_NAMESPACE_EXIT;