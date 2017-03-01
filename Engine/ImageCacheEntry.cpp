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


// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****


#include "ImageCacheEntry.h"

#include <map>

#include <boost/thread/shared_mutex.hpp> // local r-w mutex
#include <boost/thread/locks.hpp>

#include "Engine/AppManager.h"
#include "Engine/Cache.h"
#include "Engine/CacheEntryBase.h"
#include "Engine/ImageCacheKey.h"
#include "Engine/ImageStorage.h"
#include "Engine/ImagePrivate.h"
#include "Engine/ImageTilesState.h"
#include "Engine/MultiThread.h"

NATRON_NAMESPACE_ENTER;


struct FetchTile
{
    int perChannelTileIndices[4];
    TileCoord coord;

    // Either all 4 pointers are set or none.
    // If set this points to a tile in a higher scale to fetch
    boost::shared_ptr<FetchTile> upscaleTiles[4];

    FetchTile()
    : perChannelTileIndices()
    , coord()
    , upscaleTiles()
    {
        perChannelTileIndices[0] = perChannelTileIndices[1] = perChannelTileIndices[2] = perChannelTileIndices[3] = -1;
    }
};

struct ImageCacheEntryPrivate;
class ImageCacheEntryInternal;
typedef boost::shared_ptr<ImageCacheEntryInternal> ImageCacheEntryInternalPtr;

class ImageCacheEntryInternal : public CacheEntryBase
{
public:

#ifndef NATRON_CACHE_NEVER_PERSISTENT
    // When persistent, this entry is local and not shared to the threads/processes so it's fine to hold a pointer to our local entry private interface
    ImageCacheEntryPrivate* _imp;
#endif

    // The vector contains the state of the tiles for each mipmap level.
    // This is the last tiles state map that was read from the cache: any tile marked eTileStatusPending is pending
    // because someone else is computing it. Any tile marked eTileStatusNotRendered is computed by us.
    TileStateMapVector perMipMapTilesState;

#ifdef NATRON_CACHE_NEVER_PERSISTENT
    // If defined, this same object is shared across all threads, hence we need to protect data ourselves
    // Otherwise this is handled by the cache
    boost::shared_mutex perMipMapTilesStateMutex;
#else

    // If true the next call to fromMemorySegment will skip the call to readAndUpdateStateMap. This enables to update
    // the cache tiles map from within markCacheTilesAsRendered()
    bool nextFromMemorySegmentSkipUpdate;
#endif

private:

#ifndef NATRON_CACHE_NEVER_PERSISTENT
    ImageCacheEntryInternal(ImageCacheEntryPrivate* _imp)
    : CacheEntryBase(appPTR->getTileCache())
    , _imp(_imp)
    , perMipMapTilesState()
    , nextFromMemorySegmentSkipUpdate(false)
    {}
#else
    ImageCacheEntryInternal()
    : CacheEntryBase(appPTR->getTileCache())
    , perMipMapTilesState()
    , perMipMapTilesStateMutex()
    {}
#endif // NATRON_CACHE_NEVER_PERSISTENT

public:
    
#ifndef NATRON_CACHE_NEVER_PERSISTENT
    static ImageCacheEntryInternalPtr create(ImageCacheEntryPrivate* _imp, const ImageCacheKeyPtr& key)
#else
    static ImageCacheEntryInternalPtr create(const ImageCacheKeyPtr& key)
#endif
    {
#ifndef NATRON_CACHE_NEVER_PERSISTENT
        ImageCacheEntryInternalPtr ret(new ImageCacheEntryInternal(_imp));
#else
        ImageCacheEntryInternalPtr ret(new ImageCacheEntryInternal());
#endif
        ret->setKey(key);
        return ret;
    }

    virtual ~ImageCacheEntryInternal()
    {

    }

#ifndef NATRON_CACHE_NEVER_PERSISTENT
    virtual void toMemorySegment(ExternalSegmentType* segment, ExternalSegmentTypeHandleList* objectPointers) const OVERRIDE FINAL;

    virtual CacheEntryBase::FromMemorySegmentRetCodeEnum fromMemorySegment(bool isLockedForWriting,
                                                                           ExternalSegmentType* segment,
                                                                           ExternalSegmentTypeHandleList::const_iterator start,
                                                                           ExternalSegmentTypeHandleList::const_iterator end) OVERRIDE FINAL WARN_UNUSED_RETURN;
#endif


    /**
     * @brief We allow a single thread to fetch this image multiple times without computing necessarily on the first fetch.
     * Without this the cache would hang.
     **/
    virtual bool allowMultipleFetchForThread() const OVERRIDE FINAL
    {
        return true;
    }

};


inline ImageCacheEntryInternalPtr
toImageCacheEntryInternal(const CacheEntryBasePtr& entry)
{
    return boost::dynamic_pointer_cast<ImageCacheEntryInternal>(entry);
}

struct ImageCacheEntryPrivate
{
    
    // Ptr to the public interface
    ImageCacheEntry* _publicInterface;

    // The RoI of the image, rounded to the tile size
    RectI roiRoundedToTileSize;

    // Pointer to the local buffer on which to get cached data from
    Image::CPUData localData;

    // Pointer to the effect for fast abort when copying tiles from/to the cache
    EffectInstancePtr effect;

    // The mipmap level we are interested in
    unsigned int mipMapLevel;

    // The key corresponding to this ImageCacheEntry
    ImageCacheKeyPtr key;

    // State indicating the render status of each tiles
    // The state is shared accross all channels because OpenFX doesn't allows yet to render only some channels
    // This state is local to this ImageCacheEntry: any tile marked eTileStatusPending may be pending because
    // we are computing it or someone else is computing it.
    ImageTilesStatePtr state;

    // Pointer to the internal cache entry. When NATRON_CACHE_NEVER_PERSISTENT is defined, this is shared across threads/processes
    // otherwise this is local to this object.
    ImageCacheEntryInternalPtr internalCacheEntry;

    // When reading the state map, this is the list of tiles to fetch from the cache in fetchAndCopyCachedTiles()
    std::vector<FetchTile> tilesToFetch;

    ImageCacheEntryPrivate(ImageCacheEntry* publicInterface,
                           const RectI& pixelRod,
                           const RectI& roi,
                           unsigned int mipMapLevel,
                           ImageBitDepthEnum depth,
                           int nComps,
                           const void* storage[4],
                           const EffectInstancePtr& effect,
                           const ImageCacheKeyPtr& key)
    : _publicInterface(publicInterface)
    , roiRoundedToTileSize(roi)
    , localData()
    , effect(effect)
    , mipMapLevel(mipMapLevel)
    , key(key)
    , state()
    , internalCacheEntry()
    , tilesToFetch()
    {
        localData.bitDepth = depth;
        localData.bounds = roi;
        localData.nComps = nComps;
        memcpy(localData.ptrs, storage, sizeof(void*) * 4);
        assert(nComps > 0);
        int tileSizeX, tileSizeY;
        Cache::getTileSizePx(depth, &tileSizeX, &tileSizeY);
        roiRoundedToTileSize.roundToTileSize(tileSizeX, tileSizeY);
        state.reset(new ImageTilesState(pixelRod, tileSizeX, tileSizeY));

    }

    virtual ~ImageCacheEntryPrivate()
    {

    }

    enum UpdateStateMapRetCodeEnum
    {
        // The state map is up to date, it doesn't require to update the state map in the cache
        eUpdateStateMapRetCodeUpToDate,

        // The state map is not up to date, it need to be written to in the cache but we cannot do so
        // since we don't have the write lock
        eUpdateStateMapRetCodeNeedWriteLock,

        // The state map was written and updated, it must be written to the cache
        eUpdateStateMapRetCodeMustWriteToCache
    };

    /**
     * @brief Update the state of this local entry from the cache entry
     **/
    UpdateStateMapRetCodeEnum readAndUpdateStateMap(bool hasExclusiveLock);

    /**
     * @brief Fetch from the cache tiles that are cached. This must be called after a call to readAndUpdateStateMap
     * @returns true on success, false if something went wrong
     **/
    bool fetchAndCopyCachedTiles(const std::vector<FetchTile>& tilesToFetch,
                                 const Image::CPUData& toBuffer);


    TileStatusEnum lookupTileInPyramid(TileStateMapVector& perMipMapTilesState,
                                       unsigned int mipMapLevel,
                                       const TileCoord& coord,
                                       int nComps,
                                       int tileSizeX,
                                       int tileSizeY,
                                       FetchTile* tile);

};

ImageCacheEntry::ImageCacheEntry(const RectI& pixelRod,
                                 const RectI& roi,
                                 unsigned int mipMapLevel,
                                 ImageBitDepthEnum depth,
                                 int nComps,
                                 const void* storage[4],
                                 const EffectInstancePtr& effect,
                                 const ImageCacheKeyPtr& key)
: _imp(new ImageCacheEntryPrivate(this, pixelRod, roi, mipMapLevel, depth, nComps, storage, effect, key))
{
}

ImageCacheEntry::~ImageCacheEntry()
{

}

template <typename PIX>
void copyPixelsForDepth(const RectI& renderWindow,
                        const PIX* srcPixelsData,
                        int srcXStride,
                        int srcYStride,
                        PIX* dstPixelsData,
                        int dstXStride,
                        int dstYStride)
{
    const PIX* src_pixels = srcPixelsData;
    PIX* dst_pixels = dstPixelsData;

    if (srcXStride == 1 && srcXStride == dstXStride) {
        for (int y = renderWindow.y1; y < renderWindow.y2; ++y,
             src_pixels += srcYStride,
             dst_pixels += dstYStride) {

            memcpy(dst_pixels, src_pixels, renderWindow.width() * sizeof(srcXStride));
        }
    } else {
        for (int y = renderWindow.y1; y < renderWindow.y2; ++y) {
            for (int x = renderWindow.x1; x < renderWindow.x1; ++x,
                 src_pixels += srcXStride,
                 dst_pixels += dstXStride) {

                *dst_pixels = *src_pixels;
            }

            dst_pixels += (dstYStride - renderWindow.width() * dstXStride);
            src_pixels += (srcYStride - renderWindow.width() * srcXStride);
        }
    }
} // copyPixelsForDepth

static void copyPixelsForDepth(const RectI& renderWindow,
                               ImageBitDepthEnum depth,
                               const void* srcPixelsData,
                               int srcXStride,
                               int srcYStride,
                               void* dstPixelsData,
                               int dstXStride,
                               int dstYStride)
{
    switch (depth) {
        case eImageBitDepthByte:
            copyPixelsForDepth<char>(renderWindow, (const char*)srcPixelsData, srcXStride, srcYStride, (char*)dstPixelsData, dstXStride, dstYStride);
            break;
        case eImageBitDepthShort:
            copyPixelsForDepth<unsigned short>(renderWindow, (const unsigned short*)srcPixelsData, srcXStride, srcYStride, (unsigned short*)dstPixelsData, dstXStride, dstYStride);
            break;
        case eImageBitDepthFloat:
            copyPixelsForDepth<float>(renderWindow, (const float*)srcPixelsData, srcXStride, srcYStride, (float*)dstPixelsData, dstXStride, dstYStride);
            break;
        default:
            break;
    }
}

template <bool copyToCache>
class CachePixelsTransferProcessor : public MultiThreadProcessorBase
{

public:

    struct CopyData
    {
        void* ptr;
        int offset;
        RectI bounds;
        RectI roi;
    };
    
private:

    std::vector<CopyData> _tileBuffers;
    Image::CPUData _localBuffer;
public:

    CachePixelsTransferProcessor(const EffectInstancePtr& renderClone)
    : MultiThreadProcessorBase(renderClone)
    , _tileBuffers()
    , _localBuffer()
    {

    }

    virtual ~CachePixelsTransferProcessor()
    {
    }

    void setValues(const Image::CPUData& localBuffer,
                   const std::vector<CopyData>& tileBuffers)
    {
        _localBuffer = localBuffer;
        _tileBuffers = tileBuffers;
    }


    virtual ActionRetCodeEnum multiThreadFunction(unsigned int threadID,
                                                  unsigned int nThreads) OVERRIDE FINAL WARN_UNUSED_RETURN
    {

        int fromIndex, toIndex;
        ImageMultiThreadProcessorBase::getThreadRange(threadID, nThreads, 0, _tileBuffers.size(), &fromIndex, &toIndex);
        for (int i = fromIndex; i < toIndex; ++i) {

            const CopyData& tileBufData = _tileBuffers[i];

            RectI renderWindow = _localBuffer.bounds;
            tileBufData.bounds.intersect(_localBuffer.bounds, &renderWindow);


            if (copyToCache) {
                copyPixels(renderWindow, _localBuffer.bitDepth, _localBuffer.ptrs[tileBufData.offset], _localBuffer.nComps, _localBuffer.bounds.width() * _localBuffer.nComps, tileBufData.ptr, 1, tileBufData.bounds.width());
            } else {
                copyPixels(renderWindow, _localBuffer.bitDepth, tileBufData.ptr, 1, tileBufData.bounds.width(), _localBuffer.ptrs[tileBufData.offset], _localBuffer.nComps, _localBuffer.bounds.width() * _localBuffer.nComps);
            }

        }
        return eActionStatusOK;
    } // multiThreadFunction
};

typedef CachePixelsTransferProcessor<true> ToCachePixelsTransferProcessor;
typedef CachePixelsTransferProcessor<false> FromCachePixelsTransferProcessor;


struct CacheDataLock_RAII
{
    void* data;
    CachePtr cache;

    CacheDataLock_RAII(const CachePtr& cache, void* cacheData)
    : data(cacheData)
    , cache(cache)
    {

    }

    ~CacheDataLock_RAII()
    {
        cache->unLockTiles(data);
    }
};

TileStatusEnum
ImageCacheEntryPrivate::lookupTileInPyramid(TileStateMapVector& perMipMapTilesState,
                                            unsigned int lookupLevel,
                                            const TileCoord& coord,
                                            int nComps,
                                            int tileSizeX,
                                            int tileSizeY,
                                            FetchTile* tile)
{
    TileStateMap& stateMap = perMipMapTilesState[lookupLevel];

    std::vector<FetchTile> tilesToFetch;

    // Find this tile in the current state
    TileStateMap::iterator foundTile = stateMap.find(coord);
    assert(foundTile != stateMap.end());
    if (foundTile == stateMap.end()) {
        return eTileStatusNotRendered;
    }

    tile->coord = coord;


    switch (foundTile->second.status) {
        case eTileStatusRendered: {
            // If the tile has become rendered, mark it in the tiles to
            // fetch list. This will be fetched later on (when outside of the cache
            // scope) in fetchAndCopyCachedTiles()
            for (int k = 0; k < nComps; ++k) {
                tile->perChannelTileIndices[k] = foundTile->second.channelsTileStorageIndex[k];
            }
            return eTileStatusRendered;
        }
        case eTileStatusPending: {
            // If the tile is pending in the cache, leave it pending locally
            return eTileStatusPending;
        }
        case eTileStatusNotRendered: {

            // If we are in a mipmap level > 0, check in higher scales if the image is not yet available, in which
            // case we just have to downscale by a power of 2 (4 tiles become 1 tile)
            if (lookupLevel > 0) {

                int nextTx = coord.tx * 2;
                int nextTy = coord.ty * 2;
                const TileCoord upscaledCoords[4] = {
                    {nextTx, nextTy},             {nextTx + tileSizeX, nextTy},
                    {nextTx, nextTy + tileSizeY}, {nextTx + tileSizeX, nextTy + tileSizeY}
                };

                bool higherScaleTilePending = false;
                for (int i = 0; i < 4; ++i) {
                    tile->upscaleTiles[i].reset(new FetchTile);
                    TileStatusEnum higherScaleStatus = lookupTileInPyramid(perMipMapTilesState, lookupLevel - 1, upscaledCoords[i], nComps, tileSizeX, tileSizeY, tile->upscaleTiles[i].get());
                    switch (higherScaleStatus) {
                        case eTileStatusNotRendered:
                            // The higher scale tile is not rendered, we have to render anyway
                            return eTileStatusNotRendered;
                        case eTileStatusRendered:
                            // Upscaled tile is rendered, everything is ok, just fetch all upscaled tiles and downscale afterwards
                            break;
                        case eTileStatusPending:
                            higherScaleTilePending = true;
                            break;
                    }
                }
                if (higherScaleTilePending) {
                    // If one of the higher scale is pending but none is marked eTileStatusNotRendered, then mark this tile pending
                    return eTileStatusPending;
                }
                // We've got all higher scale tiles, assume it is rendered and downscale later
                return eTileStatusRendered;
            } // lookupLevel > 0

            return eTileStatusNotRendered;
        }
    } // foundTile->second.status
} // lookupTileInPyramid

ImageCacheEntryPrivate::UpdateStateMapRetCodeEnum
ImageCacheEntryPrivate::readAndUpdateStateMap(bool hasExclusiveLock)
{

    // Ensure we have at least the desired mipmap level in the cache entry
    internalCacheEntry->perMipMapTilesState.resize(mipMapLevel + 1);

    // Indicates whether we modified the cached tiles state map for this mipmap level
    bool stateMapModified = false;

    // If the tiles state at our mipmap level is empty, default initialize it
    bool cachedTilesMapInitialized = !internalCacheEntry->perMipMapTilesState[mipMapLevel].empty();
    if (!cachedTilesMapInitialized) {

        internalCacheEntry->perMipMapTilesState[mipMapLevel] = state->getTilesMap();
        stateMapModified = true;

    } else { // cachedTilesMapInitialized

        int tileSizeX, tileSizeY;
        state->getTileSize(&tileSizeX, &tileSizeY);

        // This is the current state of our image, update it according to the stateMap read from the cache
        TileStateMap& localStateMap = state->getTilesMap();

        // Clear the tiles to fetch list
        tilesToFetch.clear();

        TileStateMap& cacheStateMap = internalCacheEntry->perMipMapTilesState[mipMapLevel];

        // For each tile in the RoI (rounded to the tile size):
        // Check the tile status, only copy from the cache if rendered
        for (int ty = roiRoundedToTileSize.y1; ty < roiRoundedToTileSize.y2; ty += tileSizeY) {
            for (int tx = roiRoundedToTileSize.x1; tx < roiRoundedToTileSize.x2; tx += tileSizeX) {

                assert(tx % tileSizeX == 0 && ty % tileSizeY == 0);

                TileCoord c = {tx, ty};

                // Find this tile in the current state
                TileStateMap::iterator foundInLocalMap = localStateMap.find(c);
                assert(foundInLocalMap != localStateMap.end());
                if (foundInLocalMap == localStateMap.end()) {
                    continue;
                }

                // If the tile in the old status is already rendered, do not update
                if (foundInLocalMap->second.status == eTileStatusRendered) {
                    continue;
                }

                // Traverse the mipmaps pyramid from lower scale to higher scale to find a rendered tile and then downscale if necessary
                FetchTile tile;
                TileStatusEnum stat = lookupTileInPyramid(internalCacheEntry->perMipMapTilesState, mipMapLevel, c, localData.nComps, tileSizeX, tileSizeY, &tile);


                // Update the status of the tile according to the cache status
                TileStateMap::iterator foundInCacheMap = cacheStateMap.find(c);
                assert(foundInCacheMap != cacheStateMap.end());
                if (foundInCacheMap == cacheStateMap.end()) {
                    continue;
                }


                switch (stat) {
                    case eTileStatusRendered: {
                        // If the tile has become rendered, mark it in the tiles to
                        // fetch list. This will be fetched later on (when outside of the cache
                        // scope) in fetchAndCopyCachedTiles()
                        tilesToFetch.push_back(tile);

                        // If this tile is not yet rendered at this level but we found a higher scale mipmap,
                        // we must write this tile as pending in the cache state map
                        if (tile.upscaleTiles[0]) {
                            if (!hasExclusiveLock) {
                                return ImageCacheEntryPrivate::eUpdateStateMapRetCodeNeedWriteLock;
                            }
                            foundInCacheMap->second.status = eTileStatusPending;
                        } else {
                            // Copy the tile indices from the cache.  If we must downscale, this will be done later on in fetchAndCopyCachedTiles()
                            memcpy(foundInLocalMap->second.channelsTileStorageIndex,foundInCacheMap->second.channelsTileStorageIndex,sizeof(int) * 4);
                        }

                        // Locally, update the status to rendered
                        foundInLocalMap->second.status = eTileStatusRendered;


                    }   break;
                    case eTileStatusPending: {
                        // If the tile is pending in the cache, leave it pending locally
                        foundInLocalMap->second.status = eTileStatusPending;
                    }   break;
                    case eTileStatusNotRendered: {
                        // If the tile is marked not rendered, mark it pending in the cache
                        // but mark it not rendered for us locally
                        // Only a single thread/process can mark this tile as pending
                        // hence it must do so under a write lock (hasExclusiveLock)
                        if (!hasExclusiveLock) {
                            return ImageCacheEntryPrivate::eUpdateStateMapRetCodeNeedWriteLock;
                        }
                        foundInLocalMap->second.status = eTileStatusNotRendered;
                        foundInCacheMap->second.status = eTileStatusPending;
                        stateMapModified = true;
                    }   break;
                } // switch(stat)
            }
        }
    } // !cachedTilesMapInitialized
    
    if (stateMapModified) {
        return ImageCacheEntryPrivate::eUpdateStateMapRetCodeMustWriteToCache;
    } else {
        return ImageCacheEntryPrivate::eUpdateStateMapRetCodeUpToDate;
    }
} // readAndUpdateStateMap


static void fetchTileIndicesInPyramid(const FetchTile& tile, int nComps, std::vector<int> *tileIndicesToFetch, std::size_t* nTilesAllocNeeded)
{
    for (int c = 0; c < nComps; ++c) {
        if (tile.perChannelTileIndices[c] != -1) {
            tileIndicesToFetch->push_back(tile.perChannelTileIndices[c]);
        } else if (tile.upscaleTiles[0]) {
            // We must downscale the upscaled tiles
            ++(*nTilesAllocNeeded);
            for (int i = 0; i < 4; ++i) {
                assert(tile.upscaleTiles[i]);
                fetchTileIndicesInPyramid(*tile.upscaleTiles[i], nComps, tileIndicesToFetch, nTilesAllocNeeded);
            }
        } else {
            assert(false);
        }
    }
}

bool
ImageCacheEntryPrivate::fetchAndCopyCachedTiles(const std::vector<FetchTile>& tiles,
                                                const Image::CPUData& toBuffer)
{

    // Get a vector of tile indices to fetch from the cache directly
    std::vector<int> tileIndicesToFetch;

    // Number of tiles to allocate to downscale
    std::size_t nTilesAllocNeeded = 0;
    for (std::size_t i = 0; i < tiles.size(); ++i) {
        fetchTileIndicesInPyramid(tiles[i], localData.nComps, &tileIndicesToFetch, &nTilesAllocNeeded);
    }

    if (tileIndicesToFetch.empty() && nTilesAllocNeeded) {
        // Nothing to do
        return true;
    }

    CachePtr tileCache = internalCacheEntry->getCache();

    std::vector<std::pair<int, void*> > fetchedExistingTiles;
    std::vector<std::pair<int, void*> > allocatedTiles;
    void* cacheData;
    bool gotTiles = tileCache->retrieveAndLockTiles(internalCacheEntry, tileIndicesToFetch, nTilesAllocNeeded, &fetchedExistingTiles, &allocatedTiles, &cacheData);
    CacheDataLock_RAII cacheDataDeleter(tileCache, cacheData);
    if (!gotTiles) {
        return false;
    }


    int tileSizeX, tileSizeY;
    state->getTileSize(&tileSizeX, &tileSizeY);

    // Fetch actual tile pointers on the cache

    // Recompose bits: associate tile coords to fetched pointers
    std::vector<FromCachePixelsTransferProcessor::CopyData> tilesToCopy(fetchedExistingTiles.size());

    int fetch_i = 0;
    for (std::size_t i = 0; i < tiles.size(); ++i) {

        const TileCoord& coord = tiles[i].coord;

        RectI tileBounds;
        tileBounds.x1 = coord.tx;
        tileBounds.y1 = coord.ty;
        tileBounds.x2 = coord.tx + tileSizeX;
        tileBounds.y2 = coord.ty + tileSizeY;

        for (int c = 0; c < toBuffer.nComps; ++c, ++fetch_i) {
            FromCachePixelsTransferProcessor::CopyData &copyChunk = tilesToCopy[fetch_i];
            copyChunk.ptr = fetchedExistingTiles[fetch_i].second;
            copyChunk.bounds = tileBounds;
            // Clamp the bounds of the tile so that it does not exceed the roi
            toBuffer.bounds.intersect(tileBounds, &copyChunk.roi);
            copyChunk.offset = c;
        }

    }

    // Finally copy over multiple threads each tile
    FromCachePixelsTransferProcessor processor(effect);
    processor.setValues(toBuffer, tilesToCopy);
    ActionRetCodeEnum stat = processor.launchThreadsBlocking();
    (void)stat;

    return true;

} // fetchAndCopyCachedTiles

void
ImageCacheEntry::updateLocalTilesRenderState(TileStateMap* tileStatus, bool* hasUnRenderedTile, bool *hasPendingResults)
{


    {
        // Fetch the entry in the cache
#ifdef NATRON_CACHE_NEVER_PERSISTENT
        _imp->internalCacheEntry = ImageCacheEntryInternal::create(_imp->key);
#else
        _imp->internalCacheEntry = ImageCacheEntryInternal::create(_imp.get(), _imp->key);
#endif
        CacheEntryLockerPtr cacheAccess = _imp->internalCacheEntry->getFromCache();

        CacheEntryLocker::CacheEntryStatusEnum cacheStatus = cacheAccess->getStatus();
        while (cacheStatus == CacheEntryLocker::eCacheEntryStatusComputationPending) {
            cacheStatus = cacheAccess->waitForPendingEntry();
        }
        assert(cacheStatus == CacheEntryLocker::eCacheEntryStatusCached ||
               cacheStatus == CacheEntryLocker::eCacheEntryStatusMustCompute);

#ifdef NATRON_CACHE_NEVER_PERSISTENT
        // In non peristent mode, the entry pointer is directly cached: there's no call to fromMemorySegment/toMemorySegment
        // Emulate what is done when compiled with a persistent cache
        if (cacheStatus == CacheEntryLocker::eCacheEntryStatusCached) {
            internalCacheEntry = cacheAccess->getProcessLocalEntry();

            // First call readAndUpdateStateMap under a read lock, if needed recall under a write lock
            bool mustCallUnderWriteLock = false;
            {
                boost::unique_lock<boost::shared_mutex> readLock(_imp->internalCacheEntry->perMipMapTilesStateMutex);
                ImageCacheEntryPrivate::UpdateStateMapRetCodeEnum stat = _imp->readAndUpdateStateMap(false /*hasExlcusiveLock*/);
                switch (stat) {
                    case ImageCacheEntryPrivate::eUpdateStateMapRetCodeUpToDate:
                        // We did not write anything on the state map, we are good
                        break;
                    case ImageCacheEntryPrivate::eUpdateStateMapRetCodeMustWriteToCache:
                        // We cannot be here: it is forbidden to write the state map under a read lock
                        assert(false);
                        break;
                    case ImageCacheEntryPrivate::eUpdateStateMapRetCodeNeedWriteLock:
                        // We must update the state map but cannot do so under a read lock
                        mustCallUnderWriteLock = true;
                        break;
                }

            }
            if (mustCallUnderWriteLock) {
                boost::unique_lock<boost::shared_mutex> writeLock(_imp->internalCacheEntry->perMipMapTilesStateMutex);
                ImageCacheEntryPrivate::UpdateStateMapRetCodeEnum stat = _imp->readAndUpdateStateMap(true /*hasExlcusiveLock*/);
                assert(stat == ImageCacheEntryPrivate::eUpdateStateMapRetCodeMustWriteToCache);
                (void)stat;
            }
        }
#endif // NATRON_CACHE_NEVER_PERSISTENT

        // In persistent mode:
        // If the status is CacheEntryLocker::eCacheEntryStatusCached the state map updating was done in fromMemorySegment
        // which was called inside getFromCache()
        // If the status is CacheEntryLocker::eCacheEntryStatusMustCompute:
        // this is the first time we create the ImageCacheEntryInternal in the cache and thus we are the only thread/process here.
        // fromMemorySegment was not called, and the state map is marked with eTileStatusNotRendered everywhere.

        // In non persistent mode we have to do the same thing when it is the first time we create the entry in the cache.

        // The exclusive lock is guaranteed by the cache, we don't have to do anything externally.
        if (cacheStatus == CacheEntryLocker::eCacheEntryStatusMustCompute) {

            ImageCacheEntryPrivate::UpdateStateMapRetCodeEnum stat = _imp->readAndUpdateStateMap(true /*hasExclusiveLock*/);

            // All tiles should be eTileStatusNotRendered and thus we set them all to eTileStatusPending and must insert the results in te cache
            assert(stat == ImageCacheEntryPrivate::eUpdateStateMapRetCodeMustWriteToCache);
            (void)stat;
            cacheAccess->insertInCache();
        }
    }

    // If we found any new cached tile, fetch and copy them to our local storage
    _imp->fetchAndCopyCachedTiles();

    *hasPendingResults = false;
    *hasUnRenderedTile = false;

    const TileStateMap& stateMap = _imp->state->getTilesMap();
    for (TileStateMap::const_iterator it = stateMap.begin(); it != stateMap.end(); ++it) {
    
        if (it->second.status == eTileStatusNotRendered) {
            *hasUnRenderedTile = true;
        } else if (it->second.status == eTileStatusPending) {
            *hasPendingResults = true;
        }
    }
    if (tileStatus) {
        *tileStatus = stateMap;
    }
} // updateTilesRenderState

void
ImageCacheEntry::markCacheTilesAsRendered()
{
    // Make sure to call updateTilesRenderState() first
    assert(_imp->internalCacheEntry);

    TileStateMap& localStateMap = _imp->state->getTilesMap();

#ifdef NATRON_CACHE_NEVER_PERSISTENT
    // In non-persistent mode, lock the cache entry since it's shared across threads
    boost::unique_lock<boost::shared_mutex> writeLock(_imp->internalCacheEntry->perMipMapTilesStateMutex);
#endif

    // Ensure we have at least the desired mipmap level in the cache entry
    _imp->internalCacheEntry->perMipMapTilesState.resize(_imp->mipMapLevel + 1);
    TileStateMap& cacheStateMap = _imp->internalCacheEntry->perMipMapTilesState[_imp->mipMapLevel];
    if (cacheStateMap.empty()) {
        cacheStateMap = localStateMap;
    }

    CachePtr cache = _imp->internalCacheEntry->getCache();

    bool hasModifiedTileMap = false;

    std::vector<TileCoord> tilesToCopy;
    for (TileStateMap::iterator it = localStateMap.begin(); it != localStateMap.end(); ++it) {

        if (it->second.status == eTileStatusNotRendered) {

            it->second.status = eTileStatusRendered;

            TileStateMap::iterator foundInUpdatedCacheMap = cacheStateMap.find(it->first);
            assert(foundInUpdatedCacheMap != cacheStateMap.end());
            if (foundInUpdatedCacheMap == cacheStateMap.end()) {
                continue;
            }

            // We marked the cache tile status to eTileStatusPending previously in
            // readAndUpdateStateMap
            // Mark it as eTileStatusRendered now
            foundInUpdatedCacheMap->second.status = eTileStatusRendered;

            hasModifiedTileMap = true;

            // Mark this tile in the list of tiles to copy
            tilesToCopy.push_back(it->first);
        }
    }

    if (!hasModifiedTileMap) {
        return;
    }

    int tileSizeX, tileSizeY;
    _imp->state->getTileSize(&tileSizeX, &tileSizeY);


    std::size_t nMonoChannelBuffers = tilesToCopy.size() * _imp->localData.nComps;
    std::vector<std::pair<int, void*> > allocatedTiles;
    void* cacheData;
    bool gotTiles = cache->reserveAndLockTiles(_imp->internalCacheEntry, nMonoChannelBuffers, &allocatedTiles, &cacheData);

    CacheDataLock_RAII cacheDataDeleter(cache, cacheData);
    if (!gotTiles) {
        return;
    }

    // Associate each allocated tile to a channel tile to copy
    std::vector<ToCachePixelsTransferProcessor::CopyData> copyDataVec(allocatedTiles.size());
    {
        int alloc_i = 0;
        for (std::size_t i = 0; i < tilesToCopy.size(); ++i) {

            const TileCoord& coord = tilesToCopy[i];

            RectI tileBounds;
            tileBounds.x1 = coord.tx;
            tileBounds.y1 = coord.ty;
            tileBounds.x2 = coord.tx + tileSizeX;
            tileBounds.y2 = coord.ty + tileSizeY;

            for (int c = 0; c < _imp->localData.nComps; ++c, ++alloc_i) {
                ToCachePixelsTransferProcessor::CopyData& data = copyDataVec[alloc_i];
                data.bounds = tileBounds;
                data.ptr = allocatedTiles[alloc_i].second;
                // Clamp the bounds of the tile so that it does not exceed the roi
                _imp->localData.bounds.intersect(tileBounds, &data.roi);
                data.offset = c;
            }
        }
    }

    // Finally copy over multiple threads each tile
    ToCachePixelsTransferProcessor processor(_imp->effect);
    processor.setValues(_imp->localData, copyDataVec);
    ActionRetCodeEnum stat = processor.launchThreadsBlocking();
    (void)stat;

#ifndef NATRON_CACHE_NEVER_PERSISTENT
    {
        // Re-lookup in the cache the entry, this will force a call to fromMemorySegment, thus updating the tiles state
        // as seen in the cache.
        // There may be a possibility that the tiles state were removed from the cache for some external reasons.
        // In this case we just re-insert back the entry in the cache.

        // Flag that we don't want to update the tile state locally
        _imp->internalCacheEntry->nextFromMemorySegmentSkipUpdate = true;
        CacheEntryLockerPtr cacheAccess = _imp->internalCacheEntry->getFromCache();
        CacheEntryLocker::CacheEntryStatusEnum cacheStatus = cacheAccess->getStatus();
        while (cacheStatus == CacheEntryLocker::eCacheEntryStatusComputationPending) {
            cacheStatus = cacheAccess->waitForPendingEntry();
        }
        assert(cacheStatus == CacheEntryLocker::eCacheEntryStatusCached ||
               cacheStatus == CacheEntryLocker::eCacheEntryStatusMustCompute);

        // There may be a case where the entry was removed, insert in the cache, forcing a call to toMemorySegment
        if (cacheStatus == CacheEntryLocker::eCacheEntryStatusMustCompute) {
            cacheAccess->insertInCache();
        }

        _imp->internalCacheEntry->nextFromMemorySegmentSkipUpdate = false;

    }
#endif // NATRON_CACHE_NEVER_PERSISTENT
} // markCacheTilesAsRendered

#ifndef NATRON_CACHE_NEVER_PERSISTENT
CacheEntryBase::FromMemorySegmentRetCodeEnum
ImageCacheEntryInternal::fromMemorySegment(bool isLockedForWriting,
                                           ExternalSegmentType* segment,
                                           ExternalSegmentTypeHandleList::const_iterator start,
                                           ExternalSegmentTypeHandleList::const_iterator end) {
    // Deserialize and optionnally update the tiles state
    {

        if (start == end) {
            return CacheEntryBase::eFromMemorySegmentRetCodeFailed;
        }
        TileStateMapVector* mipMapStates = (TileStateMapVector*)segment->get_address_from_handle(*start);
        assert(mipMapStates->get_allocator().get_segment_manager() == segment->get_segment_manager());
        ++start;

        if (nextFromMemorySegmentSkipUpdate) {
            // We have written the state map from within markCacheTilesAsRendered(), update it if possible
            if (!isLockedForWriting) {
                // We must update the state map but cannot do so under a read lock
                return CacheEntryBase::eFromMemorySegmentRetCodeNeedWriteLock;
            }

            void_allocator alloc(segment->get_segment_manager());
            mipMapStates->clear();
            for (std::size_t i = 0; i < perMipMapTilesState.size(); ++i) {
                TileStateMap mipMapStateMap(alloc);
                mipMapStateMap = perMipMapTilesState[i];
                mipMapStates->push_back(mipMapStateMap);
            }
        } else {
            // Read the state map
            ImageCacheEntryPrivate::UpdateStateMapRetCodeEnum stat = _imp->readAndUpdateStateMap(isLockedForWriting);
            switch (stat) {
                case ImageCacheEntryPrivate::eUpdateStateMapRetCodeUpToDate:
                    // We did not write anything on the state map, we are good
                    break;
                case ImageCacheEntryPrivate::eUpdateStateMapRetCodeMustWriteToCache: {
                    // We have written the state map, we must update it
                    assert(isLockedForWriting);
                    void_allocator alloc(segment->get_segment_manager());
                    mipMapStates->clear();
                    for (std::size_t i = 0; i < perMipMapTilesState.size(); ++i) {
                        TileStateMap mipMapStateMap(alloc);
                        mipMapStateMap = perMipMapTilesState[i];
                        mipMapStates->push_back(mipMapStateMap);
                    }

                }   break;
                case ImageCacheEntryPrivate::eUpdateStateMapRetCodeNeedWriteLock:
                    // We must update the state map but cannot do so under a read lock
                    return CacheEntryBase::eFromMemorySegmentRetCodeNeedWriteLock;
            }
        }
    }
    
    return CacheEntryBase::fromMemorySegment(isLockedForWriting, segment, start, end);
} // fromMemorySegment


void
ImageCacheEntryInternal::toMemorySegment(ExternalSegmentType* segment, ExternalSegmentTypeHandleList* objectPointers) const
{

    // Copy the tile state to the memory segment, this will be called the first time we construct the entry in the cache
    {

        void_allocator alloc(segment->get_segment_manager());

        TileStateMapVector *mipMapStates = segment->construct<TileStateMapVector>(boost::interprocess::anonymous_instance)(alloc);
        if (!mipMapStates) {
            throw std::bad_alloc();
        }
        mipMapStates->clear();
        for (std::size_t i = 0; i < perMipMapTilesState.size(); ++i) {
            TileStateMap mipMapStateMap(alloc);
            mipMapStateMap = perMipMapTilesState[i];
            mipMapStates->push_back(mipMapStateMap);
        }

        objectPointers->push_back(segment->get_handle_from_address(mipMapStates));
    }
    CacheEntryBase::toMemorySegment(segment, objectPointers);
} // toMemorySegment
#endif // #ifndef NATRON_CACHE_NEVER_PERSISTENT

NATRON_NAMESPACE_EXIT;
