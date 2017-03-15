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

#include <QThread>
#include <QDebug>

#include <boost/thread/shared_mutex.hpp> // local r-w mutex
#include <boost/thread/locks.hpp>

#include "Engine/AppManager.h"
#include "Engine/Cache.h"
#include "Engine/CacheEntryBase.h"
#include "Engine/ImageCacheKey.h"
#include "Engine/ImageStorage.h"
#include "Engine/ImagePrivate.h"
#include "Engine/ImageCacheEntryProcessing.h"
#include "Engine/ImageTilesState.h"
#include "Engine/MultiThread.h"
#include "Engine/ThreadPool.h"

// Define to log tiles status in the console
//#define TRACE_TILES_STATUS

NATRON_NAMESPACE_ENTER;

struct TileCoord
{
    int tx,ty;
};

struct TileCoordCompare
{
    bool operator() (const TileCoord& lhs, const TileCoord& rhs) const
    {
        if (lhs.ty < rhs.ty) {
            return true;
        } else if (lhs.ty > rhs.ty) {
            return false;
        } else {
            return lhs.tx < rhs.tx;
        }
    }
};

typedef std::set<TileCoord, TileCoordCompare> TilesSet;

struct TileCacheIndex
{
    // Indices of the tiles in the cache for each channel
    U64 perChannelTileIndices[4];

    // The coord of the tile
    int tx, ty;

    // Either all 4 pointers are set or none.
    // If set this points to a tile in a higher scale to fetch
    boost::shared_ptr<TileCacheIndex> upscaleTiles[4];

    TileCacheIndex()
    : perChannelTileIndices()
    , tx(-1)
    , ty(-1)
    , upscaleTiles()
    {
        perChannelTileIndices[0] = perChannelTileIndices[1] = perChannelTileIndices[2] = perChannelTileIndices[3] = (U64)-1;
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
    std::vector<TilesState> perMipMapTilesState;

#ifdef NATRON_CACHE_NEVER_PERSISTENT
    // If defined, this same object is shared across all threads, hence we need to protect data ourselves
    // Otherwise this is handled by the cache
    boost::shared_mutex perMipMapTilesStateMutex;
#else

    // If true the next call to fromMemorySegment will skip the call to readAndUpdateStateMap. This enables to update
    // the cache tiles map from within updateCachedTilesStateMap()
    bool nextFromMemorySegmentCallIsToMemorySegment;
#endif

private:

#ifndef NATRON_CACHE_NEVER_PERSISTENT
    ImageCacheEntryInternal(ImageCacheEntryPrivate* _imp)
    : CacheEntryBase(appPTR->getTileCache())
    , _imp(_imp)
    , perMipMapTilesState()
    , nextFromMemorySegmentCallIsToMemorySegment(false)
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

// Pointers to the tile buffer + bounds of the tile + channel offset
struct TileData
{
    void* ptr;
    U64 tileCache_i;
    RectI bounds;
    int channel_i;

};

struct DownscaleTile : public TileData
{
    // Ptr to the src tiles. Non NULL when they are ready
    boost::shared_ptr<TileData> srcTiles[4];
};


struct ImageCacheEntryPrivate
{
    
    // Ptr to the public interface
    ImageCacheEntry* _publicInterface;

    // The RoI of the image
    RectI roi;


    // The image buffers, used when the localBuffers is not yet set
    ImageStorageBasePtr imageBuffers[4];

    // Local image data
    void* localBuffers[4];

    int nComps;

    int pixelStride;

    // The bitdepth
    ImageBitDepthEnum bitdepth;

    ImageBufferLayoutEnum format;

    // Pointer to the effect for fast abort when copying tiles from/to the cache
    EffectInstancePtr effect;

    // The mipmap level we are interested in
    unsigned int mipMapLevel;

    // The key corresponding to this ImageCacheEntry
    ImageCacheKeyPtr key;

    // State indicating the render status of each tiles at the mipMapLevel
    // The state is shared accross all channels because OpenFX doesn't allows yet to render only some channels
    // This state is local to this ImageCacheEntry: any tile marked eTileStatusPending may be pending because
    // we are computing it or someone else is computing it.
    TileStateHeader localTilesState;

    // Set of tiles that we marked pending in the cache that we are supposed to render now for each mipmap level
    std::vector<TilesSet> markedTiles;

    // True if waitForPendingTiles() needs to wait for updates from another render thread
    bool hasPendingTiles;

    // Pointer to the internal cache entry. When NATRON_CACHE_NEVER_PERSISTENT is defined, this is shared across threads/processes
    // otherwise this is local to this object.
    ImageCacheEntryInternalPtr internalCacheEntry;

    // When reading the state map, this is the list of tiles to fetch from the cache in fetchAndCopyCachedTiles()
    std::vector<TileCacheIndex> tilesToFetch;

    // If true the entry will be removed from the cache before reading it so we get a clean image
    bool removeFromCache;

    ImageWPtr image;

    ImageCacheEntryPrivate(ImageCacheEntry* publicInterface,
                           const ImagePtr& image,
                           const RectI& pixelRod,
                           const RectI& roi,
                           unsigned int mipMapLevel,
                           ImageBitDepthEnum depth,
                           int nComps,
                           const ImageStorageBasePtr storage[4],
                           ImageBufferLayoutEnum format,
                           const EffectInstancePtr& effect,
                           const ImageCacheKeyPtr& key,
                           bool removeFromCache)
    : _publicInterface(publicInterface)
    , roi(roi)
    , imageBuffers()
    , localBuffers()
    , nComps(nComps)
    , pixelStride(0)
    , bitdepth(depth)
    , format(format)
    , effect(effect)
    , mipMapLevel(mipMapLevel)
    , key(key)
    , localTilesState()
    , markedTiles()
    , hasPendingTiles(false)
    , internalCacheEntry()
    , tilesToFetch()
    , removeFromCache(removeFromCache)
    , image(image)
    {
        for (int i = 0; i < 4; ++i) {
            imageBuffers[i] = storage[i];
        }
        memset(localBuffers, 0, sizeof(void*) * 4);


        assert(nComps > 0);
        int tileSizeX, tileSizeY;
        Cache::getTileSizePx(depth, &tileSizeX, &tileSizeY);

#ifndef NDEBUG
        assert(pixelRod.contains(roi));
        // We must only compute rectangles that are a multiple of the tile size
        // otherwise we might end up in a situation where a partial tile could be rendered
        assert(roi.x1 % tileSizeX == 0 || roi.x1 == pixelRod.x1);
        assert(roi.y1 % tileSizeY == 0 || roi.y1 == pixelRod.y1);
        assert(roi.x2 % tileSizeX == 0 || roi.x2 == pixelRod.x2);
        assert(roi.y2 % tileSizeY == 0 || roi.y2 == pixelRod.y2);
#endif

        localTilesState.init(tileSizeX, tileSizeY, pixelRod);
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
    UpdateStateMapRetCodeEnum readAndUpdateStateMap(bool hasExclusiveLock) WARN_UNUSED_RETURN;


    /**
     * @brief Fetch from the cache tiles that are cached. This must be called after a call to readAndUpdateStateMap
     * Note that this function does the pixels transfer
     **/
    ActionRetCodeEnum fetchAndCopyCachedTiles() WARN_UNUSED_RETURN;

#ifndef NATRON_CACHE_NEVER_PERSISTENT
    void updateCachedTilesStateMap();
#endif

    enum LookupTileStateRetCodeEnum
    {
        eLookupTileStateRetCodeUpToDate,
        eLookupTileStateRetCodeUpdated,
        eLookupTileStateRetCodeNeedWriteLock
    };

    LookupTileStateRetCodeEnum lookupTileStateInPyramidRecursive(bool hasExclusiveLock,
                                                                 std::vector<TileStateHeader>& perMipMapTilesState,
                                                                 unsigned int mipMapLevel,
                                                                 TileCoord coord,
                                                                 TileCacheIndex* tile,
                                                                 TileStatusEnum* status);

    LookupTileStateRetCodeEnum lookupTileStateInPyramid(bool hasExclusiveLock,
                                                        std::vector<TileStateHeader>& perMipMapTilesState,
                                                        TileCoord coord);

    std::vector<boost::shared_ptr<TileData> > buildTaskPyramidRecursive(unsigned int lookupLevel,
                                                                        const TileCacheIndex &tile,
                                                                        const std::vector<void*> &fetchedExistingTiles,
                                                                        const std::vector<std::pair<U64, void*> >& allocatedTiles,
                                                                        int* existingTiles_i,
                                                                        int* allocatedTiles_i,
                                                                        std::vector<boost::shared_ptr<TileData> > *tilesToCopy,
                                                                        std::vector<std::vector<boost::shared_ptr<DownscaleTile> > >  *downscaleTilesPerLevel);
    void ensureImageBuffersAllocated();

};

ImageCacheEntry::ImageCacheEntry(const ImagePtr& image,
                                 const RectI& pixelRod,
                                 const RectI& roi,
                                 unsigned int mipMapLevel,
                                 ImageBitDepthEnum depth,
                                 int nComps,
                                 const ImageStorageBasePtr storage[4],
                                 ImageBufferLayoutEnum format,
                                 const EffectInstancePtr& effect,
                                 const ImageCacheKeyPtr& key,
                                 bool removeFromCache)
: _imp(new ImageCacheEntryPrivate(this, image, pixelRod, roi, mipMapLevel, depth, nComps, storage, format, effect, key, removeFromCache))
{
}

ImageCacheEntry::~ImageCacheEntry()
{
    // Ensure we unmark the pending status of tiles
    markCacheTilesAsAborted();
}

ImageCacheKeyPtr
ImageCacheEntry::getCacheKey() const
{
    return _imp->key;
}

void
ImageCacheEntryPrivate::ensureImageBuffersAllocated()
{
    image.lock()->ensureBuffersAllocated();

    if (!localBuffers[0]) {
        // Extract channel pointers
        Image::CPUData data;
        ImagePrivate::getCPUDataInternal(roi, nComps, imageBuffers, bitdepth, format, &data);
        Image::getChannelPointers((const void**)data.ptrs, roi.x1, roi.y1, roi, nComps, bitdepth, localBuffers, &pixelStride);
    }
}


class CachePixelsTransferProcessorBase : public MultiThreadProcessorBase
{
protected:
    std::vector<boost::shared_ptr<TileData> > _tasks;
    ImageCacheEntryPrivate* _imp;
public:

    CachePixelsTransferProcessorBase(const EffectInstancePtr& renderClone)
    : MultiThreadProcessorBase(renderClone)
    , _tasks()
    , _imp(0)
    {

    }

    virtual ~CachePixelsTransferProcessorBase()
    {
    }

    void setValues(ImageCacheEntryPrivate* imp,
                   const std::vector<boost::shared_ptr<TileData> >& tasks)
    {
        _imp = imp;
        _tasks = tasks;
    }
};

template <bool copyToCache, typename PIX>
class CachePixelsTransferProcessor : public CachePixelsTransferProcessorBase
{


public:

    CachePixelsTransferProcessor(const EffectInstancePtr& renderClone)
    : CachePixelsTransferProcessorBase(renderClone)
    {

    }

    virtual ~CachePixelsTransferProcessor()
    {
    }


    virtual ActionRetCodeEnum multiThreadFunction(unsigned int threadID,
                                                  unsigned int nThreads) OVERRIDE FINAL WARN_UNUSED_RETURN
    {

        
        int fromIndex, toIndex;
        ImageMultiThreadProcessorBase::getThreadRange(threadID, nThreads, 0, _tasks.size(), &fromIndex, &toIndex);
        for (int i = fromIndex; i < toIndex; ++i) {

            const TileData& task = *_tasks[i];
            
            assert(task.tileCache_i != (U64)-1);
            
            // Intersect the tile bounds
            RectI tileBoundsRounded = task.bounds;
            tileBoundsRounded.roundToTileSize(_imp->localTilesState.tileSizeX, _imp->localTilesState.tileSizeY);

            if (copyToCache) {

                // When copying to the cache, always copy full tiles, but ensure we do not copy outside of the bounds of the RoI for tiles on the border
                RectI renderWindow;
                tileBoundsRounded.intersect(_imp->roi, &renderWindow);


                const PIX* localPix = (const PIX*)Image::pixelAtStatic(renderWindow.x1, renderWindow.y1, _imp->roi, _imp->nComps, sizeof(PIX), (const unsigned char*)_imp->localBuffers[task.channel_i]);
                assert(localPix);

                PIX* tilePix = ImageCacheEntryProcessing::getPix((PIX*)task.ptr, renderWindow.x1, renderWindow.y1, tileBoundsRounded);
                assert(tilePix);
                ImageCacheEntryProcessing::copyPixelsForDepth<PIX>(renderWindow, localPix, _imp->pixelStride, _imp->roi.width() * _imp->pixelStride, tilePix, 1, _imp->localTilesState.tileSizeX);
                
                // When inserting a tile in the cache, if this is a tile in the border, repeat edges
                if (task.bounds.width() != _imp->localTilesState.tileSizeX ||
                    task.bounds.height() != _imp->localTilesState.tileSizeY) {
                    ImageCacheEntryProcessing::repeatEdgesForDepth<PIX>((PIX*)task.ptr, task.bounds, _imp->localTilesState.tileSizeX, _imp->localTilesState.tileSizeY);
                }
            } else {


                // When reading from the cache, if aborted don't continue
                if (_effect && _effect->isRenderAborted()) {
                    return eActionStatusAborted;
                }
                // When copying from the cache, clip to the tile bounds for the tiles on the border
                RectI renderWindow;
                task.bounds.intersect(_imp->roi, &renderWindow);

                const PIX* tilePix = ImageCacheEntryProcessing::getPix((const PIX*)task.ptr, renderWindow.x1, renderWindow.y1, tileBoundsRounded);
                assert(tilePix);

                PIX* localPix = (PIX*)Image::pixelAtStatic(renderWindow.x1, renderWindow.y1, _imp->roi, _imp->nComps, sizeof(PIX), (const unsigned char*)_imp->localBuffers[task.channel_i]);
                assert(localPix);

                ImageCacheEntryProcessing::copyPixelsForDepth<PIX>(renderWindow, tilePix, 1, _imp->localTilesState.tileSizeX, localPix, _imp->pixelStride, _imp->roi.width() * _imp->pixelStride);
            }

        }
        return eActionStatusOK;
    } // multiThreadFunction
};


class DownscaleMipMapProcessorBase : public MultiThreadProcessorBase
{
protected:

    std::vector<boost::shared_ptr<DownscaleTile> > _tasks;
    int _tileSizeX, _tileSizeY;
public:

    DownscaleMipMapProcessorBase(const EffectInstancePtr& renderClone)
    : MultiThreadProcessorBase(renderClone)
    , _tasks()
    , _tileSizeX(-1)
    , _tileSizeY(-1)
    {

    }

    virtual ~DownscaleMipMapProcessorBase()
    {
    }

    void setValues(int tileSizeX, int tileSizeY, const std::vector<boost::shared_ptr<DownscaleTile> >& tasks)
    {
        _tileSizeX = tileSizeX;
        _tileSizeY = tileSizeY;
        _tasks = tasks;
    }

};

template <typename PIX>
class DownscaleMipMapProcessor : public DownscaleMipMapProcessorBase
{




public:

    DownscaleMipMapProcessor(const EffectInstancePtr& renderClone)
    : DownscaleMipMapProcessorBase(renderClone)
    {

    }

    virtual ~DownscaleMipMapProcessor()
    {
    }

    virtual ActionRetCodeEnum multiThreadFunction(unsigned int threadID,
                                                  unsigned int nThreads) OVERRIDE FINAL WARN_UNUSED_RETURN
    {

        int fromIndex, toIndex;
        ImageMultiThreadProcessorBase::getThreadRange(threadID, nThreads, 0, _tasks.size(), &fromIndex, &toIndex);
        for (int i = fromIndex; i < toIndex; ++i) {

            // When reading from the cache, if aborted don't continue
            if (_effect && _effect->isRenderAborted()) {
                return eActionStatusAborted;
            }

            const DownscaleTile& task = *_tasks[i];

            const void* srcPtrs[4] = {
                task.srcTiles[0] ? task.srcTiles[0]->ptr : 0,
                task.srcTiles[1] ? task.srcTiles[1]->ptr : 0,
                task.srcTiles[2] ? task.srcTiles[2]->ptr : 0,
                task.srcTiles[3] ? task.srcTiles[3]->ptr : 0};

            ImageCacheEntryProcessing::downscaleMipMapForDepth<PIX>((const PIX**)srcPtrs, (PIX*)task.ptr, task.bounds, _tileSizeX, _tileSizeY);
        }
        return eActionStatusOK;
    } // multiThreadFunction
};

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

ImageCacheEntryPrivate::LookupTileStateRetCodeEnum
ImageCacheEntryPrivate::lookupTileStateInPyramid(bool hasExclusiveLock,
                                                    std::vector<TileStateHeader>& perMipMapTilesState,
                                                    TileCoord coord)
{
    TileCacheIndex tile;
    TileStatusEnum status;
    return lookupTileStateInPyramidRecursive(hasExclusiveLock, perMipMapTilesState, mipMapLevel, coord, &tile, &status);
}

ImageCacheEntryPrivate::LookupTileStateRetCodeEnum
ImageCacheEntryPrivate::lookupTileStateInPyramidRecursive(bool hasExclusiveLock,
                                                          std::vector<TileStateHeader>& perMipMapTilesState,
                                                          unsigned int lookupLevel,
                                                          TileCoord coord,
                                                          TileCacheIndex* tile,
                                                          TileStatusEnum* status)
{
    *status = eTileStatusNotRendered;
    
    TileStateHeader& cachedTilesState = perMipMapTilesState[lookupLevel];

    if (cachedTilesState.state->tiles.empty()) {
        // The state map may be not initialized at other mipmap levels so far but should be filled at this mipmap level
        assert(lookupLevel != mipMapLevel);
        return eLookupTileStateRetCodeUpToDate;
    }

    if (markedTiles[lookupLevel].find(coord) != markedTiles[lookupLevel].end()) {
        // We may already marked this tile before
        return eLookupTileStateRetCodeUpToDate;
    }

    // Find this tile in the current state
    TileState* cacheTileState = cachedTilesState.getTileAt(coord.tx, coord.ty);

    // There's a local tile state only for the requested mipmap level
    TileState* localTileState = lookupLevel == mipMapLevel ? localTilesState.getTileAt(coord.tx, coord.ty) : 0;
    tile->tx = coord.tx;
    tile->ty = coord.ty;


    switch (cacheTileState->status) {
        case eTileStatusRendered: {

            for (int k = 0; k < nComps; ++k) {
                tile->perChannelTileIndices[k] = cacheTileState->channelsTileStorageIndex[k];
                assert(tile->perChannelTileIndices[k] != (U64)-1);
            }

            // If the tile has become rendered at the requested mipmap level, mark it in the tiles to
            // fetch list. This will be fetched later on (when outside of the cache
            // scope) in fetchAndCopyCachedTiles()
            if (lookupLevel == mipMapLevel) {
                tilesToFetch.push_back(*tile);
                // Locally, update the status to rendered
                localTileState->status = eTileStatusRendered;
            }

            *status = eTileStatusRendered;
            return eLookupTileStateRetCodeUpToDate;
        }
        case eTileStatusPending: {
            // If the tile is pending in the cache, leave it pending locally
            if (lookupLevel == mipMapLevel) {
                localTileState->status = eTileStatusPending;
                hasPendingTiles = true;
            }
            *status = eTileStatusPending;
            return eLookupTileStateRetCodeUpToDate;

        }
        case eTileStatusNotRendered: {


            // If we are in a mipmap level > 0, check in higher scales if the image is not yet available, in which
            // case we just have to downscale by a power of 2 (4 tiles become 1 tile)
            TileStatusEnum stat = eTileStatusNotRendered;
            LookupTileStateRetCodeEnum retCode = eLookupTileStateRetCodeUpToDate;
            if (lookupLevel > 0) {

                int nextTx = coord.tx * 2;
                int nextTy = coord.ty * 2;

                /*
                 Upscale tile coords:
                 23
                 01
                 */
                TileCoord upscaledCords[4] = {
                    {nextTx, nextTy},
                    {nextTx + localTilesState.tileSizeX, nextTy},
                    {nextTx, nextTy + localTilesState.tileSizeY},
                    {nextTx + localTilesState.tileSizeX, nextTy + localTilesState.tileSizeY}
                };


                // Initialize the status to rendered
                stat = eTileStatusRendered;

#ifdef DEBUG
                int nInvalid = 0;
#endif
                for (int i = 0; i < 4; ++i) {

                    tile->upscaleTiles[i].reset(new TileCacheIndex);
                    // The upscaled tile might not exist when we are on the border, account for it
                    if (upscaledCords[i].tx < perMipMapTilesState[lookupLevel -1].bounds.x1 ||
                        upscaledCords[i].tx >= perMipMapTilesState[lookupLevel -1].bounds.x2 ||
                        upscaledCords[i].ty < perMipMapTilesState[lookupLevel -1].bounds.y1 ||
                        upscaledCords[i].ty >= perMipMapTilesState[lookupLevel -1].bounds.y2) {
#ifdef DEBUG
                        ++nInvalid;
#endif
                        continue;
                    }

                    // Recurse on the higher scale tile and get its status
                    TileStatusEnum higherScaleStatus;
                    LookupTileStateRetCodeEnum recurseRet = lookupTileStateInPyramidRecursive(hasExclusiveLock, perMipMapTilesState, lookupLevel - 1, upscaledCords[i], tile->upscaleTiles[i].get(), &higherScaleStatus);
                    switch (recurseRet) {
                        case eLookupTileStateRetCodeNeedWriteLock:
                            return eLookupTileStateRetCodeNeedWriteLock;
                        case eLookupTileStateRetCodeUpToDate:
                            break;
                        case eLookupTileStateRetCodeUpdated:
                            retCode = eLookupTileStateRetCodeUpdated;
                            break;
                    }

                    switch (higherScaleStatus) {
                        case eTileStatusNotRendered:
                            stat = eTileStatusNotRendered;
                            break;
                        case eTileStatusRendered:
                            // Higher scale tile is already rendered
                            break;
                        case eTileStatusPending:
                            stat = eTileStatusPending;
                            break;
                    }
                    if (stat == eTileStatusNotRendered) {
                        break;
                    }
                } // for each upscaled mipmap
#ifdef DEBUG
                assert(stat == eTileStatusNotRendered || (nInvalid == 0 || nInvalid == 2 || nInvalid == 3));
#endif
            } // lookupLevel > 0

            *status = stat;
            switch (stat) {
                case eTileStatusPending:
                    // One or more of the upscaled tile(s) in the pyramid is pending, but all others are rendered. Wait for them to be computed
                    if (lookupLevel == mipMapLevel) {
                        localTileState->status = eTileStatusPending;
                        hasPendingTiles = true;
                    }
                    break;
                case eTileStatusRendered:

                    // If we reach here, it is because upscaled tile are cached
                    assert(tile->upscaleTiles[0]);

                    // All 4 upscaled tiles are rendered, just fetch all upscaled tiles and downscale afterwards
                    // If this tile is not yet rendered at this level but we found a higher scale mipmap,
                    // we must write this tile as pending in the cache state map so that another thread does not attempt to downscale too
                    if (!hasExclusiveLock) {
                        return eLookupTileStateRetCodeNeedWriteLock;
                    }
                    cacheTileState->status = eTileStatusPending;
                    if (lookupLevel == mipMapLevel) {
                        // Mark it not rendered locally, this will be switched to rendered once the downscale operation is performed
                        localTileState->status = eTileStatusNotRendered;
                        tilesToFetch.push_back(*tile);
                    }

                    // Mark this tile so it is found quickly in markCacheTilesAsAborted()
#ifdef TRACE_TILES_STATUS
                    qDebug() << QThread::currentThread()  << effect->getScriptName_mt_safe().c_str() << internalCacheEntry->getHashKey() << "marking " << coord.tx << coord.ty << "pending at level" << lookupLevel;
#endif

                    markedTiles[lookupLevel].insert(coord);

                    retCode = eLookupTileStateRetCodeUpdated;

                    break;
                case eTileStatusNotRendered:
                    if (lookupLevel == mipMapLevel) {

                        // We are going to modify the state, we need exclusive rights
                        if (!hasExclusiveLock) {
                            return eLookupTileStateRetCodeNeedWriteLock;
                        }

                        // Mark it not rendered locally
                        localTileState->status = eTileStatusNotRendered;

                        // Mark it pending in the cache
                        cacheTileState->status = eTileStatusPending;

                        // Mark this tile so it is found quickly in markCacheTilesAsAborted()
#ifdef TRACE_TILES_STATUS
                        qDebug() << QThread::currentThread() << effect->getScriptName_mt_safe().c_str() << internalCacheEntry->getHashKey() << "marking " << coord.tx << coord.ty << "pending at level" << lookupLevel;
#endif
                        markedTiles[lookupLevel].insert(coord);

                        retCode = eLookupTileStateRetCodeUpdated;
                    }
                    break;
            }

            return retCode;
        } // eTileStatusNotRendered
    } // switch(foundTile->status)
} // lookupTileStateInPyramidRecursive

ImageCacheEntryPrivate::UpdateStateMapRetCodeEnum
ImageCacheEntryPrivate::readAndUpdateStateMap(bool hasExclusiveLock)
{

    // Ensure we have at least the desired mipmap level in the cache entry
    if (internalCacheEntry->perMipMapTilesState.size() < mipMapLevel + 1) {
        if (!hasExclusiveLock) {
            return ImageCacheEntryPrivate::eUpdateStateMapRetCodeNeedWriteLock;
        }
        internalCacheEntry->perMipMapTilesState.resize(mipMapLevel + 1);
    }


    // Indicates whether we modified the cached tiles state map for this mipmap level
    bool stateMapModified = false;

    // Make the map objects from the tile vectors stored in the cache
    std::vector<TileStateHeader> perMipMapCacheTilesState(mipMapLevel + 1);
    RectI mipmap0Bounds = localTilesState.bounds.upscalePowerOfTwo(mipMapLevel);
    for (std::size_t i = 0; i < perMipMapCacheTilesState.size(); ++i) {
        RectI levelBounds = mipmap0Bounds.downscalePowerOfTwo(i);
        perMipMapCacheTilesState[i] = TileStateHeader(localTilesState.tileSizeX, localTilesState.tileSizeY, levelBounds, &internalCacheEntry->perMipMapTilesState[i]);
    }

    // If the tiles state at our mipmap level is empty, default initialize it
    bool cachedTilesMapInitialized = !perMipMapCacheTilesState[mipMapLevel].state->tiles.empty();
    if (!cachedTilesMapInitialized) {
        if (!hasExclusiveLock) {
            return ImageCacheEntryPrivate::eUpdateStateMapRetCodeNeedWriteLock;
        }
        *perMipMapCacheTilesState[mipMapLevel].state = *localTilesState.state;
        stateMapModified = true;
    }

    if (markedTiles.size() != mipMapLevel + 1) {
        markedTiles.resize(mipMapLevel + 1);
    }

    // Clear the tiles to fetch list
    tilesToFetch.clear();
    hasPendingTiles = false;

    // For each tile in the RoI (rounded to the tile size):
    // Check the tile status, only copy from the cache if rendered
    for (int ty = roi.y1; ty < roi.y2; ty += localTilesState.tileSizeY) {
        for (int tx = roi.x1; tx < roi.x2; tx += localTilesState.tileSizeX) {

            assert(tx % localTilesState.tileSizeX == 0 && ty % localTilesState.tileSizeY == 0);

            TileState* localTileState = localTilesState.getTileAt(tx, ty);

            // If the tile in the old status is already rendered, do not update
            if (localTileState->status == eTileStatusRendered) {
                continue;
            }

            TileCoord coord = {tx, ty};

            // Traverse the mipmaps pyramid from lower scale to higher scale to find a rendered tile and then downscale if necessary
            LookupTileStateRetCodeEnum stat = lookupTileStateInPyramid(hasExclusiveLock, perMipMapCacheTilesState, coord);
            switch (stat) {
                case eLookupTileStateRetCodeNeedWriteLock:
                    return ImageCacheEntryPrivate::eUpdateStateMapRetCodeNeedWriteLock;
                case eLookupTileStateRetCodeUpdated:
                    stateMapModified = true;
                    break;
                case eLookupTileStateRetCodeUpToDate:
                    break;
            }
        }
    }


    if (stateMapModified) {
        return ImageCacheEntryPrivate::eUpdateStateMapRetCodeMustWriteToCache;
    } else {
        return ImageCacheEntryPrivate::eUpdateStateMapRetCodeUpToDate;
    }
} // readAndUpdateStateMap


/**
 * @brief Lookup for the given tile the tile indices in the cache.
 * If this tile needs to be computed from the average of 4 upscaled tiles, this will also request a tile
 * to be allocated and recursively call the function upstream.
 **/
static void fetchTileIndicesInPyramid(const TileCacheIndex& tile, int nComps, std::vector<U64> *tileIndicesToFetch, std::size_t* nTilesAllocNeeded)
{
    if (tile.upscaleTiles[0]) {
        // We must downscale the upscaled tiles
        *nTilesAllocNeeded = *nTilesAllocNeeded + nComps;;
        for (int i = 0; i < 4; ++i) {
            assert(tile.upscaleTiles[i]);
            // Check that the upscaled tile exists
            if (tile.upscaleTiles[i]->tx != -1) {
                fetchTileIndicesInPyramid(*tile.upscaleTiles[i], nComps, tileIndicesToFetch, nTilesAllocNeeded);
            }
        }
    } else {
        for (int c = 0; c < nComps; ++c) {
            assert(tile.perChannelTileIndices[c] != (U64)-1);
            tileIndicesToFetch->push_back(tile.perChannelTileIndices[c]);
        }
    }
} // fetchTileIndicesInPyramid

/**
 * @brief Build the tasks pyramid for a given tile
 **/
std::vector<boost::shared_ptr<TileData> >
ImageCacheEntryPrivate::buildTaskPyramidRecursive(unsigned int lookupLevel,
                                                  const TileCacheIndex &tile,
                                                  const std::vector<void*> &fetchedExistingTiles,
                                                  const std::vector<std::pair<U64, void*> >& allocatedTiles,
                                                  int* existingTiles_i,
                                                  int* allocatedTiles_i,
                                                  std::vector<boost::shared_ptr<TileData> > *tilesToCopy,
                                                  std::vector<std::vector<boost::shared_ptr<DownscaleTile> > >  *downscaleTilesPerLevel)
{

    std::vector<boost::shared_ptr<TileData> > outputTasks(nComps);

    RectI thisLevelRoD;
    if (lookupLevel == mipMapLevel) {
        thisLevelRoD = localTilesState.bounds;
    } else {
        RectI mipmap0RoD = localTilesState.bounds.upscalePowerOfTwo(mipMapLevel);
        thisLevelRoD = mipmap0RoD.downscalePowerOfTwo(lookupLevel);
    }
    // The tile bounds are clipped to the pixel RoD
    RectI tileBounds;
    tileBounds.x1 = std::max(tile.tx, thisLevelRoD.x1);
    tileBounds.y1 = std::max(tile.ty, thisLevelRoD.y1);
    tileBounds.x2 = std::min(tile.tx + localTilesState.tileSizeX, thisLevelRoD.x2);
    tileBounds.y2 = std::min(tile.ty + localTilesState.tileSizeY, thisLevelRoD.y2);
    assert(!tileBounds.isNull());
    assert(downscaleTilesPerLevel->size() > lookupLevel);

    if (tile.upscaleTiles[0]) {

        // Recurse upstream.
        // For each 4 upscaled tiles, it returns us a vector of exactly nComps tasks.
        std::vector<boost::shared_ptr<TileData> > upscaledTasks[4];
#ifdef DEBUG
        int nInvalidTiles = 0;
#endif
        for (int i = 0; i < 4; ++i) {
            assert(tile.upscaleTiles[i]);

            // Check if the upscale tile exists, if so recurse
            if (tile.upscaleTiles[i]->tx != -1) {
                upscaledTasks[i] = buildTaskPyramidRecursive(lookupLevel - 1, *tile.upscaleTiles[i],  fetchedExistingTiles, allocatedTiles, existingTiles_i, allocatedTiles_i, tilesToCopy, downscaleTilesPerLevel);
                assert((int)upscaledTasks[i].size() == nComps);
            }
#ifdef DEBUG
            else {
                ++nInvalidTiles;
            }
#endif
        }
#ifdef DEBUG
        assert(nInvalidTiles == 0 || nInvalidTiles == 2 || nInvalidTiles == 3);
#endif

        // Create tasks at this level
        for (int c = 0; c < nComps; ++c) {
            boost::shared_ptr<DownscaleTile> task(new DownscaleTile);
            task->bounds = tileBounds;
            assert(*allocatedTiles_i >= 0 && *allocatedTiles_i < (int)allocatedTiles.size());
            task->ptr = allocatedTiles[*allocatedTiles_i].second;
            task->tileCache_i = allocatedTiles[*allocatedTiles_i].first;
            task->channel_i = c;
            ++(*allocatedTiles_i);
            for (int i = 0; i < 4; ++i) {
                if (!upscaledTasks[i].empty()) {
                    task->srcTiles[i] = upscaledTasks[i][c];
                }
            }

            (*downscaleTilesPerLevel)[lookupLevel].push_back(task);
        }
        
    } else { // !tile.upscaleTiles[0]
        for (int c = 0; c < nComps; ++c) {
            assert(tile.perChannelTileIndices[c] != (U64)-1);


            outputTasks[c].reset(new TileData);
            assert(*existingTiles_i >= 0 && *existingTiles_i < (int)fetchedExistingTiles.size());
            outputTasks[c]->ptr = fetchedExistingTiles[*existingTiles_i];
            ++(*existingTiles_i);
            assert(tile.perChannelTileIndices[c] != (U64)-1);
            outputTasks[c]->tileCache_i = tile.perChannelTileIndices[c];
            outputTasks[c]->bounds = tileBounds;
            outputTasks[c]->channel_i = c;

            // If we are at the level of interest, add the tile to the copy tasks list
            if (lookupLevel == mipMapLevel) {
                tilesToCopy->push_back(outputTasks[c]);
            }
        }
        
    } // tile.upscaleTiles[0]
    return outputTasks;
} // buildTaskPyramidRecursive

ActionRetCodeEnum
ImageCacheEntryPrivate::fetchAndCopyCachedTiles()
{

    // Get a vector of tile indices to fetch from the cache directly
    std::vector<U64> tileIndicesToFetch;

    // Number of tiles to allocate to downscale
    std::size_t nTilesAllocNeeded = 0;
    for (std::size_t i = 0; i < tilesToFetch.size(); ++i) {
        fetchTileIndicesInPyramid(tilesToFetch[i], nComps, &tileIndicesToFetch, &nTilesAllocNeeded);
    }

    if (tileIndicesToFetch.empty() && !nTilesAllocNeeded) {
        // Nothing to do
        return eActionStatusOK;
    }

    // We are going to fetch data from the cache, ensure our local buffers are allocated
    ensureImageBuffersAllocated();

    // Get the tile pointers on the cache
    CachePtr tileCache = internalCacheEntry->getCache();

    // List of the tiles we need to copy
    std::vector<void*> fetchedExistingTiles;

    // Allocated buffers for tiles
    std::vector<std::pair<U64, void*> > allocatedTiles;
    void* cacheData;
    bool gotTiles = tileCache->retrieveAndLockTiles(internalCacheEntry, &tileIndicesToFetch, nTilesAllocNeeded, &fetchedExistingTiles, &allocatedTiles, &cacheData);
    CacheDataLock_RAII cacheDataDeleter(tileCache, cacheData);
    if (!gotTiles) {
        return eActionStatusFailed;
    }


    // Fetch actual tile pointers on the cache

    // Recompose bits: associate tile coords to fetched pointers
    std::vector<boost::shared_ptr<TileData> > tilesToCopy;

    // For each mipmap level the list of downscale tiles
    std::vector<std::vector<boost::shared_ptr<DownscaleTile> > > perLevelTilesToDownscale(mipMapLevel + 1);

    {
        int existingTiles_i = 0;
        int allocatedTiles_i = 0;
        for (std::size_t i = 0; i < tilesToFetch.size(); ++i) {
            buildTaskPyramidRecursive(mipMapLevel, tilesToFetch[i], fetchedExistingTiles, allocatedTiles, &existingTiles_i, &allocatedTiles_i, &tilesToCopy, &perLevelTilesToDownscale);
        }
    }

    // If we downscaled some tiles, we updated the tiles status map
    bool stateMapUpdated = false;

    // Downscale in parallel each mipmap level tiles and then copy the last level tiles
    RectI mipmap0Bounds = localTilesState.bounds.upscalePowerOfTwo(mipMapLevel);
    for (std::size_t i = 0; i < perLevelTilesToDownscale.size(); ++i) {

        if (perLevelTilesToDownscale[i].empty()) {
            continue;
        }
        RectI levelBounds = mipmap0Bounds.downscalePowerOfTwo(i);
        TileStateHeader cacheStateMap = TileStateHeader(localTilesState.tileSizeX, localTilesState.tileSizeY, levelBounds, &internalCacheEntry->perMipMapTilesState[i]);
        assert(!cacheStateMap.state->tiles.empty());



        // Downscale all tiles for the same mipmap level concurrently
        boost::scoped_ptr<DownscaleMipMapProcessorBase> processor;
        switch (bitdepth) {
            case eImageBitDepthByte:
                processor.reset(new DownscaleMipMapProcessor<unsigned char>(effect));
                break;
            case eImageBitDepthShort:
                processor.reset(new DownscaleMipMapProcessor<unsigned short>(effect));
                break;
            case eImageBitDepthFloat:
                processor.reset(new DownscaleMipMapProcessor<float>(effect));
                break;
            default:
                break;
        }
        processor->setValues(localTilesState.tileSizeX, localTilesState.tileSizeY, perLevelTilesToDownscale[i]);
        ActionRetCodeEnum stat = processor->launchThreadsBlocking();
        if (isFailureRetCode(stat)) {
            return stat;
        }

        stateMapUpdated = true;

        // We downscaled hence we must update tiles status from eTileStatusNotRendered to eTileStatusRendered
        // Only do so for the first channel since they all share the same state
#ifdef NATRON_CACHE_NEVER_PERSISTENT
        // In non-persistent mode, lock the cache entry since it's shared across threads.
        // In persistent mode the entry is copied in fromMemorySegment
        boost::unique_lock<boost::shared_mutex> writeLock(internalCacheEntry->perMipMapTilesStateMutex);
#endif

        for (std::size_t j = 0; j  < perLevelTilesToDownscale[i].size(); ++j) {

            const RectI& tileBounds = perLevelTilesToDownscale[i][j]->bounds;

            // Get the bottom left coordinates of the tile
            int tx = (int)std::floor((double)tileBounds.x1 / localTilesState.tileSizeX) * localTilesState.tileSizeX;
            int ty = (int)std::floor((double)tileBounds.y1 / localTilesState.tileSizeY) * localTilesState.tileSizeY;


            TileState* cacheTileState = cacheStateMap.getTileAt(tx, ty);
            assert(perLevelTilesToDownscale[i][j]->tileCache_i != (U64)-1);
            cacheTileState->channelsTileStorageIndex[perLevelTilesToDownscale[i][j]->channel_i] = perLevelTilesToDownscale[i][j]->tileCache_i;

#ifdef DEBUG
            // Check that upon the last channels, all tile indices are correct
            if (perLevelTilesToDownscale[i][j]->channel_i == nComps -1) {
                for (int c = 0; c < nComps; ++c) {
                    assert(cacheTileState->channelsTileStorageIndex[c] != (U64)-1);
                }
            }
#endif
            
            // Update the tile state only for the first channel
            if (perLevelTilesToDownscale[i][j]->channel_i != 0) {
                continue;
            }

            assert(cacheTileState->status == eTileStatusPending);
            cacheTileState->status = eTileStatusRendered;

            // Remove this tile from the marked tiles
            assert(markedTiles.size() == mipMapLevel + 1);
            TileCoord coord = {tx, ty};
            TilesSet::iterator foundMarked = markedTiles[i].find(coord);
            assert(foundMarked != markedTiles[i].end());
            markedTiles[i].erase(foundMarked);
#ifdef TRACE_TILES_STATUS
            qDebug() << QThread::currentThread() << effect->getScriptName_mt_safe().c_str() << internalCacheEntry->getHashKey() << "marking " << tx << ty << "rendered with downscale at level" << i;
#endif

            // Update the state locally if we are on the appropriate mip map level
            if (i == mipMapLevel) {
                TileState* localTileState = localTilesState.getTileAt(tx, ty);
                assert(perLevelTilesToDownscale[i][j]->tileCache_i != (U64)-1);
                localTileState->channelsTileStorageIndex[perLevelTilesToDownscale[i][j]->channel_i] = perLevelTilesToDownscale[i][j]->tileCache_i;

                assert(localTileState->status == eTileStatusNotRendered);
                localTileState->status = eTileStatusRendered;
            }
        } // for each tile

    } // for each mip map level

    // In persistent mode we have to actually copy the states map from the cache entry to the cache
#ifndef NATRON_CACHE_NEVER_PERSISTENT
    if (stateMapUpdated) {
        updateCachedTilesStateMap();
    } // stateMapUpdated
#endif // NATRON_CACHE_NEVER_PERSISTENT


    // Now add the tiles we just downscaled to our level to the list of tiles to copy
    tilesToCopy.insert(tilesToCopy.end(), perLevelTilesToDownscale[mipMapLevel].begin(), perLevelTilesToDownscale[mipMapLevel].end());
    
    // Finally copy over multiple threads each tile
    boost::scoped_ptr<CachePixelsTransferProcessorBase> processor;
    switch (bitdepth) {
        case eImageBitDepthByte:
            processor.reset(new CachePixelsTransferProcessor<false /*copyToCache*/, unsigned char>(effect));
            break;
        case eImageBitDepthShort:
            processor.reset(new CachePixelsTransferProcessor<false /*copyToCache*/, unsigned short>(effect));
            break;
        case eImageBitDepthFloat:
            processor.reset(new CachePixelsTransferProcessor<false /*copyToCache*/, float>(effect));
            break;
        default:
            break;
    }
    processor->setValues(this, tilesToCopy);
    ActionRetCodeEnum stat = processor->launchThreadsBlocking();
    return stat;

} // fetchAndCopyCachedTiles

ActionRetCodeEnum
ImageCacheEntry::fetchCachedTilesAndUpdateStatus(TileStateHeader* tileStatus, bool* hasUnRenderedTile, bool *hasPendingResults)
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

        if (_imp->removeFromCache && cacheStatus == CacheEntryLocker::eCacheEntryStatusCached) {
            _imp->internalCacheEntry->getCache()->removeEntry(_imp->internalCacheEntry);
            cacheAccess = _imp->internalCacheEntry->getFromCache();
            while (cacheStatus == CacheEntryLocker::eCacheEntryStatusComputationPending) {
                cacheStatus = cacheAccess->waitForPendingEntry();
            }
            assert(cacheStatus == CacheEntryLocker::eCacheEntryStatusCached ||
                   cacheStatus == CacheEntryLocker::eCacheEntryStatusMustCompute);


            _imp->localTilesState.init(_imp->localTilesState.tileSizeX, _imp->localTilesState.tileSizeY, _imp->localTilesState.bounds);
            _imp->removeFromCache = false;
        }
#ifdef NATRON_CACHE_NEVER_PERSISTENT
        // In non peristent mode, the entry pointer is directly cached: there's no call to fromMemorySegment/toMemorySegment
        // Emulate what is done when compiled with a persistent cache
        if (cacheStatus == CacheEntryLocker::eCacheEntryStatusCached) {
            _imp->internalCacheEntry = toImageCacheEntryInternal(cacheAccess->getProcessLocalEntry());

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
#ifdef NATRON_CACHE_NEVER_PERSISTENT
            boost::unique_lock<boost::shared_mutex> writeLock(_imp->internalCacheEntry->perMipMapTilesStateMutex);
#endif
            ImageCacheEntryPrivate::UpdateStateMapRetCodeEnum stat = _imp->readAndUpdateStateMap(true /*hasExclusiveLock*/);

            // All tiles should be eTileStatusNotRendered and thus we set them all to eTileStatusPending and must insert the results in te cache
            assert(stat == ImageCacheEntryPrivate::eUpdateStateMapRetCodeMustWriteToCache);
            (void)stat;
            cacheAccess->insertInCache();
        }
    }

    if (!_imp->tilesToFetch.empty()) {
#ifdef NATRON_CACHE_NEVER_PERSISTENT
        boost::unique_lock<boost::shared_mutex> writeLock(_imp->internalCacheEntry->perMipMapTilesStateMutex);
#endif

        // If we found any new cached tile, fetch and copy them to our local storage
        ActionRetCodeEnum stat = _imp->fetchAndCopyCachedTiles();
        if (isFailureRetCode(stat)) {
            return stat;
        }

    }

    getStatus(tileStatus, hasUnRenderedTile, hasPendingResults);
    return eActionStatusOK;
} // fetchCachedTilesAndUpdateStatus

void
ImageCacheEntry::getStatus(TileStateHeader* tileStatus, bool* hasUnRenderedTile, bool *hasPendingResults) const
{

    if (tileStatus) {
        *tileStatus = TileStateHeader(_imp->localTilesState.tileSizeX, _imp->localTilesState.tileSizeY, _imp->localTilesState.bounds, _imp->localTilesState.state);
    }
    assert((!hasPendingResults && !hasUnRenderedTile) || (hasPendingResults && hasUnRenderedTile));

    if (!hasPendingResults && !hasUnRenderedTile) {
        return;
    }

    *hasPendingResults = _imp->hasPendingTiles;
    *hasUnRenderedTile = _imp->markedTiles.size() > 0;
} // getStatus

#ifndef NATRON_CACHE_NEVER_PERSISTENT
void
ImageCacheEntryPrivate::updateCachedTilesStateMap()
{
    // Re-lookup in the cache the entry, this will force a call to fromMemorySegment, thus updating the tiles state
    // as seen in the cache.
    // There may be a possibility that the tiles state were removed from the cache for some external reasons.
    // In this case we just re-insert back the entry in the cache.

    // Flag that we don't want to update the tile state locally
    internalCacheEntry->nextFromMemorySegmentCallIsToMemorySegment = true;
    CacheEntryLockerPtr cacheAccess = internalCacheEntry->getFromCache();
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

    internalCacheEntry->nextFromMemorySegmentCallIsToMemorySegment = false;
}
#endif // #ifndef NATRON_CACHE_NEVER_PERSISTENT

void
ImageCacheEntry::markCacheTilesAsAborted()
{
    // Make sure to call updateTilesRenderState() first
    assert(_imp->internalCacheEntry);

    if (_imp->markedTiles.empty()) {
        return;
    }

#ifdef NATRON_CACHE_NEVER_PERSISTENT
    // In non-persistent mode, lock the cache entry since it's shared across threads.
    // In persistent mode the entry is copied in fromMemorySegment
    boost::unique_lock<boost::shared_mutex> writeLock(_imp->internalCacheEntry->perMipMapTilesStateMutex);
#endif
    // We should have gotten the state map from the cache in fetchCachedTilesAndUpdateStatus()
    assert(!_imp->internalCacheEntry->perMipMapTilesState.empty());


    // Read the cache map and update our local map
    CachePtr cache = _imp->internalCacheEntry->getCache();
    bool hasModifiedTileMap = false;

    RectI mipmap0Bounds = _imp->localTilesState.bounds.upscalePowerOfTwo(_imp->mipMapLevel);
    for (std::size_t i = 0; i < _imp->markedTiles.size(); ++i) {

        RectI levelBounds = mipmap0Bounds.downscalePowerOfTwo(i);
        TileStateHeader cacheStateMap = TileStateHeader(_imp->localTilesState.tileSizeX, _imp->localTilesState.tileSizeY, levelBounds, &_imp->internalCacheEntry->perMipMapTilesState[i]);
        if (cacheStateMap.state->tiles.empty()) {
            // The state map may be not initialized at other mipmap levels so far but should be filled at this mipmap level
            assert(i != _imp->mipMapLevel);
            continue;
        }

        for (TilesSet::iterator it = _imp->markedTiles[i].begin(); it != _imp->markedTiles[i].end(); ++it) {


            TileState* cacheTileState = cacheStateMap.getTileAt(it->tx, it->ty);


            // We marked the cache tile status to eTileStatusPending previously in
            // readAndUpdateStateMap
            // Mark it as eTileStatusNotRendered now
            assert(cacheTileState->status == eTileStatusPending);
            cacheTileState->status = eTileStatusNotRendered;
            hasModifiedTileMap = true;
#ifdef TRACE_TILES_STATUS
            qDebug() << QThread::currentThread() << _imp->effect->getScriptName_mt_safe().c_str() << _imp->internalCacheEntry->getHashKey() << "marking " << it->tx << it->ty << "unrendered at level" << i;
#endif
            if (i == _imp->mipMapLevel) {
                TileState* localTileState = _imp->localTilesState.getTileAt(it->tx, it->ty);
                assert(localTileState->status == eTileStatusNotRendered);
                if (localTileState->status == eTileStatusNotRendered) {

                    localTileState->status = eTileStatusNotRendered;
                }
            }
        }
    }
    
    
    _imp->markedTiles.clear();
    
    if (!hasModifiedTileMap) {
        return;
    }

    // In persistent mode we have to actually copy the cache entry tiles state map to the cache
#ifndef NATRON_CACHE_NEVER_PERSISTENT
    _imp->updateCachedTilesStateMap();
#endif // NATRON_CACHE_NEVER_PERSISTENT

} // markCacheTilesAsAborted

void
ImageCacheEntry::markCacheTilesAsRendered()
{
    // Make sure to call updateTilesRenderState() first
    assert(_imp->internalCacheEntry);


    if (_imp->markedTiles.empty()) {
        return;
    }

#ifdef NATRON_CACHE_NEVER_PERSISTENT
    // In non-persistent mode, lock the cache entry since it's shared across threads.
    // In persistent mode the entry is copied in fromMemorySegment
    boost::unique_lock<boost::shared_mutex> writeLock(_imp->internalCacheEntry->perMipMapTilesStateMutex);
#endif
    // We should have gotten the state map from the cache in fetchCachedTilesAndUpdateStatus()
    assert(!_imp->internalCacheEntry->perMipMapTilesState.empty());

    assert(!_imp->localTilesState.state->tiles.empty());


    // Read the cache map and update our local map
    CachePtr cache = _imp->internalCacheEntry->getCache();
    bool hasModifiedTileMap = false;

    std::vector<boost::shared_ptr<TileData> > tilesToCopy;

    RectI mipmap0Bounds = _imp->localTilesState.bounds.upscalePowerOfTwo(_imp->mipMapLevel);
    for (std::size_t i = 0; i < _imp->markedTiles.size(); ++i) {

        RectI levelBounds = mipmap0Bounds.downscalePowerOfTwo(i);
        TileStateHeader cacheStateMap = TileStateHeader(_imp->localTilesState.tileSizeX, _imp->localTilesState.tileSizeY, levelBounds, &_imp->internalCacheEntry->perMipMapTilesState[i]);
        if (cacheStateMap.state->tiles.empty()) {
            // The state map may be not initialized at other mipmap levels so far but should be filled at this mipmap level
            assert(i != _imp->mipMapLevel);
            continue;
        }

        for (TilesSet::iterator it = _imp->markedTiles[i].begin(); it != _imp->markedTiles[i].end(); ++it) {

            TileState* cacheTileState = cacheStateMap.getTileAt(it->tx, it->ty);

            // We marked the cache tile status to eTileStatusPending previously in
            // readAndUpdateStateMap
            // Mark it as eTileStatusRendered now
            assert(cacheTileState->status == eTileStatusPending);
            cacheTileState->status = eTileStatusRendered;

#ifdef TRACE_TILES_STATUS
            qDebug() << QThread::currentThread() << _imp->effect->getScriptName_mt_safe().c_str() << _imp->internalCacheEntry->getHashKey() <<  "marking " << it->tx << it->ty << "rendered at level" << i;
#endif
            hasModifiedTileMap = true;

            if (i == _imp->mipMapLevel) {
                TileState* localTileState = _imp->localTilesState.getTileAt(it->tx, it->ty);
                assert(localTileState->status == eTileStatusNotRendered);
                if (localTileState->status == eTileStatusNotRendered) {

                    localTileState->status = eTileStatusRendered;

                    for (int c = 0; c < _imp->nComps; ++c) {
                        // Mark this tile in the list of tiles to copy
                        boost::shared_ptr<TileData> copy(new TileData);
                        copy->bounds = localTileState->bounds;
                        copy->channel_i = c;
                        tilesToCopy.push_back(copy);
                    }
                }
            }
        }
    }

    _imp->markedTiles.clear();

#ifdef DEBUG
    // Check that all tiles are marked either rendered or pending
    for (int ty = _imp->roi.y1; ty < _imp->roi.y2; ty += _imp->localTilesState.tileSizeY) {
        for (int tx = _imp->roi.x1; tx < _imp->roi.x2; tx += _imp->localTilesState.tileSizeX) {

            assert(tx % _imp->localTilesState.tileSizeX == 0 && ty % _imp->localTilesState.tileSizeY == 0);
            TileState* localTileState = _imp->localTilesState.getTileAt(tx, ty);
            assert(localTileState->status == eTileStatusPending || localTileState->status == eTileStatusRendered);
        }
    }
#endif

    if (!hasModifiedTileMap) {
        return;
    }

    // We are going to fetch data from the cache, ensure our local buffers are allocated
    _imp->ensureImageBuffersAllocated();


    std::size_t nMonoChannelBuffers = tilesToCopy.size();

    // Allocated buffers for tiles
    std::vector<std::pair<U64, void*> > allocatedTiles;
    void* cacheData;
    bool gotTiles = cache->retrieveAndLockTiles(_imp->internalCacheEntry, 0 /*existingTiles*/, nMonoChannelBuffers, NULL, &allocatedTiles, &cacheData);
    CacheDataLock_RAII cacheDataDeleter(cache, cacheData);
    if (!gotTiles) {
        return;
    }

    // This is the tiles state at the mipmap level of interest in the cache
    TileStateHeader cacheStateMap(_imp->localTilesState.tileSizeX, _imp->localTilesState.tileSizeY, _imp->localTilesState.bounds, &_imp->internalCacheEntry->perMipMapTilesState[_imp->mipMapLevel]);
    assert(!cacheStateMap.state->tiles.empty());

    // Set the tile pointer to the tiles to copy
    for (std::size_t i = 0; i < tilesToCopy.size(); ++i) {
        tilesToCopy[i]->ptr = allocatedTiles[i].second;

        tilesToCopy[i]->tileCache_i = allocatedTiles[i].first;
        // update the tile indices
        int tx = (int)std::floor((double)tilesToCopy[i]->bounds.x1 / _imp->localTilesState.tileSizeX) * _imp->localTilesState.tileSizeX;
        int ty = (int)std::floor((double)tilesToCopy[i]->bounds.y1 / _imp->localTilesState.tileSizeY) * _imp->localTilesState.tileSizeY;
        TileState* cacheTileState = cacheStateMap.getTileAt(tx, ty);
        assert(allocatedTiles[i].first != (U64)-1);
        cacheTileState->channelsTileStorageIndex[tilesToCopy[i]->channel_i] = allocatedTiles[i].first;

        TileState* localTileState = _imp->localTilesState.getTileAt(tx, ty);
        localTileState->channelsTileStorageIndex[tilesToCopy[i]->channel_i] = allocatedTiles[i].first;
    }


    // Finally copy over multiple threads each tile
    boost::scoped_ptr<CachePixelsTransferProcessorBase> processor;
    switch (_imp->bitdepth) {
        case eImageBitDepthByte:
            processor.reset(new CachePixelsTransferProcessor<true /*copyToCache*/, unsigned char>(_imp->effect));
            break;
        case eImageBitDepthShort:
            processor.reset(new CachePixelsTransferProcessor<true /*copyToCache*/, unsigned short>(_imp->effect));
            break;
        case eImageBitDepthFloat:
            processor.reset(new CachePixelsTransferProcessor<true /*copyToCache*/, float>(_imp->effect));
            break;
        default:
            break;
    }

    processor->setValues(_imp.get(), tilesToCopy);
    ActionRetCodeEnum stat = processor->launchThreadsBlocking();

    // We never abort when copying tiles to the cache since they are anyway already rendered.
    assert(stat != eActionStatusAborted);
    (void)stat;

    // In persistent mode we have to actually copy the cache entry tiles state map to the cache
#ifndef NATRON_CACHE_NEVER_PERSISTENT
    _imp->updateCachedTilesStateMap();
#endif // NATRON_CACHE_NEVER_PERSISTENT
} // markCacheTilesAsRendered

bool
ImageCacheEntry::waitForPendingTiles()
{
    if (!_imp->hasPendingTiles) {
        return true;
    }

    // When the cache is persistent, we don't hold a lock on the entry since it would also require locking
    // some mutexes protecting the memory mapping of the cache itself.
    //
    // For more explanation see comments in CacheEntryLocker::waitForPendingEntry:
    // Instead we implement polling

    // If this thread is a threadpool thread, it may wait for a while that results gets available.
    // Release the thread to the thread pool so that it may use this thread for other runnables
    // and reserve it back when done waiting.
    bool hasReleasedThread = false;
    if (isRunningInThreadPoolThread()) {
        QThreadPool::globalInstance()->releaseThread();
        hasReleasedThread = true;
    }

    std::size_t timeSpentWaitingForPendingEntryMS = 0;
    std::size_t timeToWaitMS = 40;


    bool hasUnrenderedTile;
    bool hasPendingResults;

    do {
        hasUnrenderedTile = false;
        hasPendingResults = false;
        ActionRetCodeEnum stat = fetchCachedTilesAndUpdateStatus(NULL, &hasUnrenderedTile, &hasPendingResults);
        if (isFailureRetCode(stat)) {
            return true;
        }

        if (hasPendingResults) {

            timeSpentWaitingForPendingEntryMS += timeToWaitMS;
            CacheEntryLocker::sleep_milliseconds(timeToWaitMS);

            // Increase the time to wait at the next iteration
            timeToWaitMS *= 1.2;


        }

    } while(hasPendingResults && !hasUnrenderedTile && !_imp->effect->isRenderAborted());

    if (hasReleasedThread) {
        QThreadPool::globalInstance()->reserveThread();
    }
    return !hasPendingResults && !hasUnrenderedTile;

} // waitForPendingTiles

#ifndef NATRON_CACHE_NEVER_PERSISTENT

static void fromMemorySegmentInternal(const IPCMipMapTileStateVector& cachedMipMapStates, std::vector<TilesState>* localMipMapStates)
{
    localMipMapStates->clear();
    for (std::size_t i = 0; i < cachedMipMapStates.size(); ++i) {
        TilesState localState;
        localState.tiles.insert(localState.tiles.end(), cachedMipMapStates[i].tiles.begin(), cachedMipMapStates[i].tiles.end());
        localMipMapStates->push_back(localState);
    }
}

/**
 * @brief The implementation that copies the tile status from the local state map to the cache state map
 * @param copyPendingStatusToCache If true then we blindly copy the status to the cache. If false, we only
 * update tiles that are locally marked rendered or not rendered.
 **/
static void toMemorySegmentInternal(bool copyPendingStatusToCache,
                                    const std::vector<TilesState>& localMipMapStates,
                                    IPCMipMapTileStateVector* cachedMipMapStates,
                                    ExternalSegmentType* segment)
{
    if (cachedMipMapStates->size() != localMipMapStates.size()) {
        TileState_Allocator_ExternalSegment alloc(segment->get_segment_manager());
        IPCTilesState init(alloc);
        cachedMipMapStates->resize(localMipMapStates.size(), init);
    }
    for (std::size_t i = 0; i < localMipMapStates.size(); ++i) {
        //
        IPCTilesState &cacheState = (*cachedMipMapStates)[i];
        assert(cacheState.tiles.get_allocator().get_segment_manager() == segment->get_segment_manager());

        if (cacheState.tiles.empty() || copyPendingStatusToCache) {
            cacheState.tiles.clear();
            cacheState.tiles.insert(cacheState.tiles.end(), localMipMapStates[i].tiles.begin(), localMipMapStates[i].tiles.end());
        } else {
            assert(cacheState.tiles.size() == localMipMapStates[i].tiles.size() || localMipMapStates[i].tiles.empty());
            if (localMipMapStates[i].tiles.empty()) {
                // The tiles list may be empty if we are not at our mipmap level of interest
                continue;
            }
            TileStateVector::const_iterator localIt = localMipMapStates[i].tiles.begin();
            IPCTileStateVector::iterator cacheIt = cacheState.tiles.begin();
            for (;localIt != localMipMapStates[i].tiles.end(); ++localIt, ++cacheIt) {
                
                // Do not write over rendered tiles
                if (cacheIt->status == eTileStatusRendered) {
                    continue;
                }
                if (localIt->status == eTileStatusNotRendered || localIt->status == eTileStatusRendered) {
                    *cacheIt = *localIt;
                }
            }
        }
        assert(cachedMipMapStates->back().tiles.get_allocator().get_segment_manager() == segment->get_segment_manager());
    }
}

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

        // Read our tiles state vector
        IPCMipMapTileStateVector* mipMapStates = (IPCMipMapTileStateVector*)segment->get_address_from_handle(*start);
        assert(mipMapStates->get_allocator().get_segment_manager() == segment->get_segment_manager());
        ++start;

        if (nextFromMemorySegmentCallIsToMemorySegment) {
            // We have written the state map from within markCacheTilesAsRendered(), update it if possible
            if (!isLockedForWriting) {
                // We must update the state map but cannot do so under a read lock
                return CacheEntryBase::eFromMemorySegmentRetCodeNeedWriteLock;
            }

            // If we are here, we are either in markCacheTilesAsAborted() or markCacheTilesAsRendered().
            // We may have read pending tiles from the cache that are no longer pending in the cache and we don't want to write over it
            // hence we do not update tiles that are marked pending
            toMemorySegmentInternal(false /*copyPendingStatusToCache*/, perMipMapTilesState, mipMapStates, segment);
        } else {

            // Copy the tiles states locally
            fromMemorySegmentInternal(*mipMapStates, &perMipMapTilesState);

            // Read the state map: it might modify it by marking tiles pending
            ImageCacheEntryPrivate::UpdateStateMapRetCodeEnum stat = _imp->readAndUpdateStateMap(isLockedForWriting);
            switch (stat) {
                case ImageCacheEntryPrivate::eUpdateStateMapRetCodeUpToDate:
                    // We did not write anything on the state map, we are good
                    break;
                case ImageCacheEntryPrivate::eUpdateStateMapRetCodeMustWriteToCache: {
                    // We have written the state map, we must update it
                    assert(isLockedForWriting);
                    toMemorySegmentInternal(true /*copyPendingStatusToCache*/, perMipMapTilesState, mipMapStates, segment);

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

        TileStateVector_Allocator_ExternalSegment alloc(segment->get_segment_manager());

        IPCMipMapTileStateVector *mipMapStates = segment->construct<IPCMipMapTileStateVector>(boost::interprocess::anonymous_instance)(alloc);
        if (!mipMapStates) {
            throw std::bad_alloc();
        }
        toMemorySegmentInternal(true /*copyPendingStatusToCache*/, perMipMapTilesState, mipMapStates, segment);
        objectPointers->push_back(segment->get_handle_from_address(mipMapStates));
    }
    CacheEntryBase::toMemorySegment(segment, objectPointers);
} // toMemorySegment

#endif // #ifndef NATRON_CACHE_NEVER_PERSISTENT

NATRON_NAMESPACE_EXIT;
