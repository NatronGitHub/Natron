/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2013-2017 INRIA and Alexandre Gauthier-Foichat
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

#ifndef NATRON_ENGINE_IMAGECACHEENTRY_H
#define NATRON_ENGINE_IMAGECACHEENTRY_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/scoped_ptr.hpp>
#endif

#include "Engine/ImageCacheKey.h"
#include "Engine/ImageTilesState.h"

NATRON_NAMESPACE_ENTER;

struct ImageCacheEntryPrivate;

/**
 * @brief Interface between an image and the cache.
 * This is the object that lives in the cache.
 * A storage is passed in parameter that is assumed to of the size of the
 * roi and the cache data is copied to this storage.
 * This object is not thread safe and is not meant to be shared across threads. 
 * However internally the cached tiles are thread safe and each tile is guaranteed to be
 * rendered by a single thread/process.
 **/
class ImageCacheEntry 
{
public:

    /**
     * @brief An image cache entry associated to an image.
     * @param pixelRod is the maximal size the image could have at the mipMapLevel
     * @param roi is the region we are interested in for this image
     * @param mipMapLevel Indicates the mipmap we are interested in. The rectangles are scaled to this level
     * @param depth The bitdepth of the image
     * @param nComps the number of channels in the image
     * @param Pointers to the channels storage in RAM
     * @param effect Pointer to the effect to abort quickly
     * @param key The key corresponding to this entry
     **/
    ImageCacheEntry(const RectI& pixelRod,
                    const RectI& roi,
                    unsigned int mipMapLevel,
                    ImageBitDepthEnum depth,
                    int nComps,
                    const void* storage[4],
                    const EffectInstancePtr& effect,
                    const ImageCacheKeyPtr& key);

    ~ImageCacheEntry();
    
    /**
     * @brief Fetch possibly cached tiles from the cache and update the tiles state for the given mipmap level.
     * This function will mark unrendered tiles as being rendered: any other thread/process trying to access them will
     * wait for this thread to finish.
     * @param tileStatus[out] If non null, will be set to the local tiles status map
     * @param hasUnRenderedTile[out] If set to true, there's at least one tile left to render in the roi given in the ctor
     * @param hasPendingResults[out] If set to true, then the caller should, after rendering the given rectangles
     * call waitForPendingTiles() and then afterwards recheck the rectangles left to render with this functino
     **/
    void fetchCachedTilesAndUpdateStatus(TileStateMap* tileStatus, bool* hasUnRenderedTile, bool *hasPendingResults);

    /**
     * @brief Should be called once the effect rendered successfully.
     * This function transfers the local pixels to the cache and also updates the tiles state map in the cache.
     * This will also notify any other effect waiting for these tiles.
     * Do not call if the render was aborted otherwise non-rendered pixels will be pushed to the cache and marked as rendered.
     **/
    void markCacheTilesAsRendered();

    /**
     * @brief Should be called after markCacheTilesAsRendered() to wait for any pending tiles to be rendered.
     * Once returning from that function, all tiles should be computed, but you should make a last call to
     * fetchCachedTilesAndUpdateStatus to ensure that everything is rendered. In some rare cases you may have to compute
     * tiles that were marked pending but that were aborted by another thread.
     **/
    void waitForPendingTiles();

private:

    boost::scoped_ptr<ImageCacheEntryPrivate> _imp;
};
NATRON_NAMESPACE_EXIT;

#endif // NATRON_ENGINE_IMAGECACHEENTRY_H
