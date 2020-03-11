/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * Copyright (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
 * Copyright (C) 2018-2020 The Natron developers
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

#include "Engine/EngineFwd.h"
#include "Engine/ImageCacheKey.h"
#include "Engine/ImageTilesState.h"

NATRON_NAMESPACE_ENTER

struct ImageCacheEntryPrivate;

/**
 * @brief Interface between an image and the cache.
 * This is the object that interacts with the cache.
 * A storage is passed in parameter that is assumed to be of the size of the
 * roi and the cache data is copied to/from this storage.
 * Internally the cached tiles are thread safe and each tile is guaranteed to be
 * rendered by a single thread/process.
 *
 * Note that this class may be used even without interaction with the cache, to help
 * synchronize multiple threads rendering a single image.
 **/
class ImageCacheEntry 
{
public:

    /**
     * @brief An image cache entry associated to an image.
     * @param mipMapPixelRods is the maximal size the image could have at each mipmap level up to the mipMapLevel given in parameter. The vector must be of size mipMapLevel + 1
     * We use a vector because we cannot compute them only by upscaling the RoD at the given mipmap level: the RoD might change given the mipmap level (e.g Blur)
     * @param roi is the region we are interested in for this image
     * @param isDraft Are we rendering a low quality version of the image ?
     * @param mipMapLevel Indicates the mipmap we are interested in. The roi/pixelRod are scaled to this level
     * @param depth The bitdepth of the image
     * @param nComps the number of channels in the image (1 to 4)
     * @param Pointers to the buffer(s) storage in RAM
     * @param format The layout of the image buffer(s)
     * @param effect Pointer to the effect to abort quickly
     * @param key The key corresponding to this entry
     * @param cachePolicy If set to eCacheAccessModeWriteOnly the entry will be removed from the cache before reading it so we get a clean image
     *                    If set to eCacheAccessModeNone, a local tiles state will be created without looking up the cache.
     *                    If set to eCacheAccessModeReadWrite, the entry will be read from the cache and written to if needed
     **/
    ImageCacheEntry(const ImagePtr& image,
                    const std::vector<RectI>& mipMapPixelRods,
                    const RectI& roi,
                    bool isDraft,
                    unsigned int mipMapLevel,
                    ImageBitDepthEnum depth,
                    int nComps,
                    const ImageStorageBasePtr storage[4],
                    ImageBufferLayoutEnum format,
                    const EffectInstancePtr& effect,
                    const ImageCacheKeyPtr& key,
                    CacheAccessModeEnum cachePolicy);

    ~ImageCacheEntry();


    ImageCacheKeyPtr getCacheKey() const;

    /**
     * @brief Ensure the given RoI is tracked by the tiles state map. This function does not grow the associated storage
     * and assumes that the caller has taken care that the storage size matches the unioned roi
     * Note that this function only works if this ImageCacheEntry has a cachePolicy of eCacheAccessModeNone.
     * @param roi The ImageCacheEntry will grow its internal state to include at least this area
     * @param perMipMapLevelRoDPixel For each mipmap level, the new RoD in pixel coordinates
     **/
    void ensureRoI(const RectI& roi,
                   const ImageStorageBasePtr storage[4],
                   const std::vector<RectI>& perMipMapLevelRoDPixel);

    // Change the image pointer and render clone. Do not use, for debug only
    void updateRenderCloneAndImage(const ImagePtr& image, const EffectInstancePtr& newRenderClone);


    /**
     * @brief Fetch possibly cached tiles from the cache and update the tiles state for the given mipmap level.
     * This function will mark unrendered tiles as being rendered: any other thread/process trying to access them will
     * wait for this thread to finish.
     * @param readOnly If true, this function will not mark any tile pending, even if there's still tiles to compute
     * @param tileStatus[out] If non null, will be set to the local tiles status map
     * @param hasUnRenderedTile[out] If set to true, there's at least one tile left to render in the roi given in the ctor
     * @param hasPendingResults[out] If set to true, then the caller should, after rendering the given rectangles
     * call waitForPendingTiles() and then afterwards recheck the rectangles left to render with this function
     **/
    ActionRetCodeEnum fetchCachedTilesAndUpdateStatus(bool readOnly, TileStateHeader* tileStatus, bool* hasUnRenderedTile, bool *hasPendingResults);

    /**
     * @brief Same as fetchCachedTilesAndUpdateStatus except that it does not fetch anything from the cache, it just returns previously
     * available data resulting from a previous call of fetchCachedTilesAndUpdateStatus
     **/
    void getStatus(TileStateHeader* tileStatus, bool* hasUnRenderedTile, bool *hasPendingResults) const;

    /**
     * @brief Should be called once the effect rendered successfully.
     * This function transfers the local pixels to the cache and also updates the tiles state map in the cache.
     * This will also notify any other effect waiting for these tiles.
     * Do not call if the render was aborted otherwise non-rendered pixels will be pushed to the cache and marked as rendered.
     **/
    void markCacheTilesAsRendered();

    /**
     * @brief This function should be called if the render was aborted to mark tiles that were marked pending
     * in an unrendered state.
     * If not called when a render is aborted, this may stall the application. For safety this is called in the dtor of this class.
     **/
    void markCacheTilesAsAborted();

    /**
     * @brief Same as markCacheTilesAsAborted() except that all tiles in the given RoI are marked eTileStatusNotRendered
     * instead of only tiles that were marked eTileStatusPending
     **/
    void markCacheTilesInRegionAsNotRendered(const RectI& roi);

    /**
     * @brief Should be called after markCacheTilesAsRendered() to wait for any pending tiles to be rendered.
     * Once returning from that function, all tiles should be computed, but you should make a last call to
     * fetchCachedTilesAndUpdateStatus to ensure that everything is rendered. In some rare cases you may have to compute
     * tiles that were marked pending but that were aborted by another thread.
     *
     * @returns True if everything is done, false if the caller should check again the status with fetchCachedTilesAndUpdateStatus
     **/
    bool waitForPendingTiles();

private:

    boost::scoped_ptr<ImageCacheEntryPrivate> _imp;
};
NATRON_NAMESPACE_EXIT

#endif // NATRON_ENGINE_IMAGECACHEENTRY_H
