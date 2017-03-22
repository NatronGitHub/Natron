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


class ImageCacheEntryInternalBase;
typedef boost::shared_ptr<ImageCacheEntryInternalBase> ImageCacheEntryInternalBasePtr;

class ImageCacheEntryInternalBase : public CacheEntryBase
{
public:


    // The vector contains the state of the tiles for each mipmap level.
    // This is the last tiles state map that was read from the cache: any tile marked eTileStatusPending is pending
    // because someone else is computing it. Any tile marked eTileStatusNotRendered is computed by us.
    std::vector<TilesState> perMipMapTilesState;



public:

    ImageCacheEntryInternalBase()
    : CacheEntryBase(appPTR->getTileCache())
    , perMipMapTilesState()
    {

    }

    virtual ~ImageCacheEntryInternalBase()
    {

    }


    virtual void toMemorySegment(IPCPropertyMap* properties) const OVERRIDE FINAL;

    virtual CacheEntryBase::FromMemorySegmentRetCodeEnum fromMemorySegment(bool isLockedForWriting,
                                                                           const IPCPropertyMap& properties) OVERRIDE FINAL;


    /**
     * @brief We allow a single thread to fetch this image multiple times without computing necessarily on the first fetch.
     * Without this the cache would hang.
     **/
    virtual bool allowMultipleFetchForThread() const OVERRIDE FINAL
    {
        return true;
    }

};

template <bool persistent>
class ImageCacheEntryInternal : public ImageCacheEntryInternalBase
{
};


template <>
class ImageCacheEntryInternal<true> : public ImageCacheEntryInternalBase
{
public:

    // When persistent, this entry is local and not shared to the threads/processes so it's fine to hold a pointer to our local entry private interface
    ImageCacheEntryPrivate* _imp;

    // If true the next call to fromMemorySegment will skip the call to readAndUpdateStateMap. This enables to update
    // the cache tiles map from within updateCachedTilesStateMap()
    bool nextFromMemorySegmentCallIsToMemorySegment;

private:
    ImageCacheEntryInternal(ImageCacheEntryPrivate* _imp)
    : ImageCacheEntryInternalBase()
    , _imp(_imp)
    , nextFromMemorySegmentCallIsToMemorySegment(false)
    {}

public:



    static ImageCacheEntryInternalBasePtr create(ImageCacheEntryPrivate* _imp, const ImageCacheKeyPtr& key)
    {
        ImageCacheEntryInternalBasePtr ret(new ImageCacheEntryInternal<true>(_imp));
        ret->setKey(key);
        return ret;

    }
};

template <>
class ImageCacheEntryInternal<false> : public ImageCacheEntryInternalBase
{
public:

    // If defined, this same object is shared across all threads, hence we need to protect data ourselves
    // Otherwise this is handled by the cache
    boost::shared_mutex perMipMapTilesStateMutex;


private:

    ImageCacheEntryInternal()
    : ImageCacheEntryInternalBase()
    , perMipMapTilesStateMutex()
    {}

public:

    virtual ~ImageCacheEntryInternal()
    {

    }

    static ImageCacheEntryInternalBasePtr create(const ImageCacheKeyPtr& key)
    {
        ImageCacheEntryInternalBasePtr ret(new ImageCacheEntryInternal<false>());
        ret->setKey(key);
        return ret;

    }
};


inline ImageCacheEntryInternalBasePtr
toImageCacheEntryInternal(const CacheEntryBasePtr& entry)
{
    return boost::dynamic_pointer_cast<ImageCacheEntryInternalBase>(entry);
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

    // The RoI of the image, protected by lock
    RectI roi;

    // The image buffers, used when the localBuffers is not yet set
    ImageStorageBasePtr imageBuffers[4];

    int nComps;

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
    // Protected by lock
    TileStateHeader localTilesState;

    // Set of tiles that we marked pending in the cache that we are supposed to render now for each mipmap level
    // Protected by lock
    std::vector<TilesSet> markedTiles;

    // True if waitForPendingTiles() needs to wait for updates from another render thread
    // Protected by lock
    bool hasPendingTiles;

    // Protects all members across multiple threads using the same image
    boost::mutex lock;

    // Pointer to the internal cache entry.
    // The entry is protected by the lock to ensure multiple threads using the same
    // ImagePtr do not concurrently write/read the same map.
    ImageCacheEntryInternalBasePtr internalCacheEntry;

    // When reading the state map, this is the list of tiles to fetch from the cache in fetchAndCopyCachedTiles()
    // Protected by lock
    std::vector<TileCacheIndex> tilesToFetch;

    // If set to eCacheAccessModeWriteOnly the entry will be removed from the cache before reading it so we get a clean image
    // If set to eCacheAccessModeNone, a local tiles state will be created without looking up the cache.
    // If set to eCacheAccessModeReadWrite, the entry will be read from the cache and written to if needed
    CacheAccessModeEnum cachePolicy;

    // Pointer to the image holding this ImageCacheEntry
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
                           CacheAccessModeEnum cachePolicy)
    : _publicInterface(publicInterface)
    , roi(roi)
    , imageBuffers()
    , nComps(nComps)
    , bitdepth(depth)
    , format(format)
    , effect(effect)
    , mipMapLevel(mipMapLevel)
    , key(key)
    , localTilesState()
    , markedTiles()
    , hasPendingTiles(false)
    , lock()
    , internalCacheEntry()
    , tilesToFetch()
    , cachePolicy(cachePolicy)
    , image(image)
    {
        for (int i = 0; i < 4; ++i) {
            imageBuffers[i] = storage[i];
        }

        assert(nComps > 0);
        int tileSizeX, tileSizeY;
        CacheBase::getTileSizePx(depth, &tileSizeX, &tileSizeY);

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

        // If we don't want to use the cache, still create a local object used to sync threads
        if (cachePolicy == eCacheAccessModeNone) {
            internalCacheEntry = ImageCacheEntryInternal<false>::create(key);
        }
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
        eUpdateStateMapRetCodeMustWriteToCache,

        // The tiles map was corrupted
        eUpdateStateMapRetCodeFailed

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

    void updateCachedTilesStateMap();

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
                                 CacheAccessModeEnum cachePolicy)
: _imp(new ImageCacheEntryPrivate(this, image, pixelRod, roi, mipMapLevel, depth, nComps, storage, format, effect, key, cachePolicy))
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
ImageCacheEntry::ensureRoI(const RectI& roi)
{
    boost::unique_lock<boost::mutex> locker(_imp->lock);
    _imp->roi = roi;
}


class CachePixelsTransferProcessorBase : public MultiThreadProcessorBase
{
protected:
    std::vector<boost::shared_ptr<TileData> > _tasks;
    ImageCacheEntryPrivate* _imp;
    void* _localBuffers[4];
    int _pixelStride;
public:

    CachePixelsTransferProcessorBase(const EffectInstancePtr& renderClone)
    : MultiThreadProcessorBase(renderClone)
    , _tasks()
    , _imp(0)
    , _localBuffers()
    , _pixelStride(0)
    {
        memset(_localBuffers, 0, sizeof(void*) * 4);
    }

    virtual ~CachePixelsTransferProcessorBase()
    {
    }

    void setValues(ImageCacheEntryPrivate* imp,
                   const std::vector<boost::shared_ptr<TileData> >& tasks)
    {
        _imp = imp;
        _tasks = tasks;

        // Extract channel pointers
        Image::CPUData data;
        ImagePrivate::getCPUDataInternal(_imp->roi, _imp->nComps, _imp->imageBuffers, _imp->bitdepth, _imp->format, &data);
        Image::getChannelPointers((const void**)data.ptrs, _imp->roi.x1, _imp->roi.y1, _imp->roi, _imp->nComps, _imp->bitdepth, _localBuffers, &_pixelStride);
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


                const PIX* localPix = (const PIX*)Image::pixelAtStatic(renderWindow.x1, renderWindow.y1, _imp->roi, _imp->nComps, sizeof(PIX), (const unsigned char*)_localBuffers[task.channel_i]);
                assert(localPix);

                PIX* tilePix = ImageCacheEntryProcessing::getPix((PIX*)task.ptr, renderWindow.x1, renderWindow.y1, tileBoundsRounded);
                assert(tilePix);
                ImageCacheEntryProcessing::copyPixelsForDepth<PIX>(renderWindow, localPix, _pixelStride, _imp->roi.width() * _pixelStride, tilePix, 1, _imp->localTilesState.tileSizeX);
                
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

                PIX* localPix = (PIX*)Image::pixelAtStatic(renderWindow.x1, renderWindow.y1, _imp->roi, _imp->nComps, sizeof(PIX), (const unsigned char*)_localBuffers[task.channel_i]);
                assert(localPix);

                ImageCacheEntryProcessing::copyPixelsForDepth<PIX>(renderWindow, tilePix, 1, _imp->localTilesState.tileSizeX, localPix, _pixelStride, _imp->roi.width() * _pixelStride);
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
    CacheBasePtr cache;

    CacheDataLock_RAII(const CacheBasePtr& cache, void* cacheData)
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


    if (markedTiles[lookupLevel].find(coord) != markedTiles[lookupLevel].end()) {
        // We may already marked this tile before
        return eLookupTileStateRetCodeUpToDate;
    }

    // Find this tile in the current state
    TileState* cacheTileState = cachedTilesState.getTileAt(coord.tx, coord.ty);
    *status = cacheTileState->status;
    assert(cacheTileState);

    // There's a local tile state only for the requested mipmap level
    TileState* localTileState = lookupLevel == mipMapLevel ? localTilesState.getTileAt(coord.tx, coord.ty) : 0;
    tile->tx = coord.tx;
    tile->ty = coord.ty;


    switch (*status) {
        case eTileStatusRendered: {
            assert(cacheTileState);
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
            assert(cacheTileState);
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
                    if (upscaledCords[i].tx < perMipMapTilesState[lookupLevel -1].boundsRoundedToTileSize.x1 ||
                        upscaledCords[i].tx >= perMipMapTilesState[lookupLevel -1].boundsRoundedToTileSize.x2 ||
                        upscaledCords[i].ty < perMipMapTilesState[lookupLevel -1].boundsRoundedToTileSize.y1 ||
                        upscaledCords[i].ty >= perMipMapTilesState[lookupLevel -1].boundsRoundedToTileSize.y2) {
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

    // Initialize tile state vectors stored in the cache
    std::vector<TileStateHeader> perMipMapCacheTilesState(mipMapLevel + 1);
    RectI mipmap0Bounds = localTilesState.bounds.upscalePowerOfTwo(mipMapLevel);
    for (std::size_t i = 0; i < perMipMapCacheTilesState.size(); ++i) {
        RectI levelBounds = mipmap0Bounds.downscalePowerOfTwo(i);
        RectI boundsRounded = levelBounds;
        boundsRounded.roundToTileSize(localTilesState.tileSizeX, localTilesState.tileSizeY);


        // If the cached entry does not contain the exact amount of tiles, fail: a thread may have crashed while creating the map.
        std::size_t expectedNumTiles = (boundsRounded.width() / localTilesState.tileSizeX) * (boundsRounded.height() / localTilesState.tileSizeY);
        if (!internalCacheEntry->perMipMapTilesState[i].tiles.empty() && (internalCacheEntry->perMipMapTilesState[i].tiles.size() != expectedNumTiles)) {
            return eUpdateStateMapRetCodeFailed;
        }

        // Wrap the cache entry perMipMapTilesState with a TileStateHeader
        perMipMapCacheTilesState[i] = TileStateHeader(localTilesState.tileSizeX, localTilesState.tileSizeY, levelBounds, &internalCacheEntry->perMipMapTilesState[i]);

        if (internalCacheEntry->perMipMapTilesState[i].tiles.empty()) {

            if (!hasExclusiveLock) {
                return ImageCacheEntryPrivate::eUpdateStateMapRetCodeNeedWriteLock;
            }

            // The state for this mipmap level is not yet initialized in the cache entry
            // If this is the mipmap level requested, we already initialized the state in the ctor when
            // creating localTilesState
            if (i == mipMapLevel) {
                *perMipMapCacheTilesState[i].state = *localTilesState.state;
            } else {
                // If the tiles state at the mipmap level is empty, default initialize it
                TileStateHeader tmp;
                tmp.init(localTilesState.tileSizeX, localTilesState.tileSizeY, levelBounds);
                *perMipMapCacheTilesState[i].state = *tmp.state;
            }
#ifdef TRACE_TILES_STATUS
            qDebug() << QThread::currentThread()  << effect->getScriptName_mt_safe().c_str() << internalCacheEntry->getHashKey() << "init tiles at level " << i;
#endif
            assert(!perMipMapCacheTilesState[i].state->tiles.empty());
            stateMapModified = true;
        }
        
    }


    if (markedTiles.size() != mipMapLevel + 1) {
        markedTiles.resize(mipMapLevel + 1);
    }

    // Clear the tiles to fetch list
    tilesToFetch.clear();
    hasPendingTiles = false;

    // For each tile in the RoI (rounded to the tile size):
    // Check the tile status, only copy from the cache if rendered
    RectI roiRounded = roi;
    roiRounded.roundToTileSize(localTilesState.tileSizeX, localTilesState.tileSizeY);
    for (int ty = roiRounded.y1; ty < roiRounded.y2; ty += localTilesState.tileSizeY) {
        for (int tx = roiRounded.x1; tx < roiRounded.x2; tx += localTilesState.tileSizeX) {

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
static void fetchTileIndicesInPyramid(unsigned int lookupLevel, const TileCacheIndex& tile, int nComps, std::vector<U64> *tileIndicesToFetch, std::vector<U64>* tilesAllocNeeded)
{
    if (tile.upscaleTiles[0]) {
        // We must downscale the upscaled tiles
        for (int c = 0; c < nComps; ++c) {
            U64 tileBucketHash = tile.tx + tile.ty + lookupLevel + c;
            tilesAllocNeeded->push_back(tileBucketHash);
        }
        for (int i = 0; i < 4; ++i) {
            assert(tile.upscaleTiles[i]);
            // Check that the upscaled tile exists
            if (tile.upscaleTiles[i]->tx != -1) {
                fetchTileIndicesInPyramid(lookupLevel - 1, *tile.upscaleTiles[i], nComps, tileIndicesToFetch, tilesAllocNeeded);
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
            outputTasks[c] = task;
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
    std::vector<U64> tilesAllocNeeded;
    for (std::size_t i = 0; i < tilesToFetch.size(); ++i) {
        fetchTileIndicesInPyramid(mipMapLevel, tilesToFetch[i], nComps, &tileIndicesToFetch, &tilesAllocNeeded);
    }

    if (tileIndicesToFetch.empty() && tilesAllocNeeded.empty()) {
        // Nothing to do
        return eActionStatusOK;
    }

    // We are going to fetch data from the cache, ensure our local buffers are allocated
    image.lock()->ensureBuffersAllocated();

    // Get the tile pointers on the cache
    CacheBasePtr tileCache = internalCacheEntry->getCache();

    // List of the tiles we need to copy
    std::vector<void*> fetchedExistingTiles;

    // Allocated buffers for tiles
    std::vector<std::pair<U64, void*> > allocatedTiles;
    void* cacheData;
    bool gotTiles = tileCache->retrieveAndLockTiles(internalCacheEntry, &tileIndicesToFetch, &tilesAllocNeeded, &fetchedExistingTiles, &allocatedTiles, &cacheData);
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

        boost::scoped_ptr<boost::unique_lock<boost::shared_mutex> > writeLock;
        if (!internalCacheEntry->isPersistent()) {
            // In non-persistent mode, lock the cache entry since it's shared across threads.
            // In persistent mode the entry is copied in fromMemorySegment
            ImageCacheEntryInternal<false>* nonPersistentLocalEntry = dynamic_cast<ImageCacheEntryInternal<false>* >(internalCacheEntry.get());
            assert(nonPersistentLocalEntry);
            writeLock.reset(new boost::unique_lock<boost::shared_mutex>(nonPersistentLocalEntry->perMipMapTilesStateMutex));
        }


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
    if (internalCacheEntry->isPersistent() && stateMapUpdated) {
        updateCachedTilesStateMap();
    } // stateMapUpdated


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

    // Protect all local structures against multiple threads using this object.
    {
        boost::unique_lock<boost::mutex> locker(_imp->lock);

        // Get the current number of marked tiles so that we can track if readAndUpdateStateMap() did anything
        std::vector<std::size_t> markedTilesSize(_imp->mipMapLevel + 1);
        for (std::size_t i = 0; i < markedTilesSize.size(); ++i) {
            if (i < _imp->markedTiles.size()) {
                markedTilesSize[i] = _imp->markedTiles[i].size();
            }
        }


        if (_imp->cachePolicy == eCacheAccessModeNone) {
            // When not interacting with the cache, the internalCacheEntry is actual local to this object
            ImageCacheEntryPrivate::UpdateStateMapRetCodeEnum stat = _imp->readAndUpdateStateMap(true /*hasExlcusiveLock*/);
            if (stat == ImageCacheEntryPrivate::eUpdateStateMapRetCodeFailed) {
                return eActionStatusFailed;
            }
        } else {
            // If we want interaction with the cache, we need to fetch the actual tiles state map from the cache

            if (!appPTR->getTileCache()->isPersistent()) {
                _imp->internalCacheEntry = ImageCacheEntryInternal<false>::create(_imp->key);
            } else {
                _imp->internalCacheEntry = ImageCacheEntryInternal<true>::create(_imp.get(), _imp->key);
            }

            CacheEntryLockerBasePtr cacheAccess = _imp->internalCacheEntry->getFromCache();

            CacheEntryLockerBase::CacheEntryStatusEnum cacheStatus = cacheAccess->getStatus();
            while (cacheStatus == CacheEntryLockerBase::eCacheEntryStatusComputationPending) {
                cacheStatus = cacheAccess->waitForPendingEntry();
            }
            assert(cacheStatus == CacheEntryLockerBase::eCacheEntryStatusCached ||
                   cacheStatus == CacheEntryLockerBase::eCacheEntryStatusMustCompute);

            if (_imp->cachePolicy == eCacheAccessModeWriteOnly && cacheStatus == CacheEntryLockerBase::eCacheEntryStatusCached) {
                _imp->internalCacheEntry->getCache()->removeEntry(_imp->internalCacheEntry);

                if (!appPTR->getTileCache()->isPersistent()) {
                    _imp->internalCacheEntry = ImageCacheEntryInternal<false>::create(_imp->key);
                } else {
                    _imp->internalCacheEntry = ImageCacheEntryInternal<true>::create(_imp.get(), _imp->key);
                }
                _imp->markedTiles.clear();
                _imp->tilesToFetch.clear();
                
                cacheAccess = _imp->internalCacheEntry->getFromCache();
                cacheStatus = cacheAccess->getStatus();
                while (cacheStatus == CacheEntryLockerBase::eCacheEntryStatusComputationPending) {
                    cacheStatus = cacheAccess->waitForPendingEntry();
                }
                assert(cacheStatus == CacheEntryLockerBase::eCacheEntryStatusCached ||
                       cacheStatus == CacheEntryLockerBase::eCacheEntryStatusMustCompute);


                _imp->localTilesState.init(_imp->localTilesState.tileSizeX, _imp->localTilesState.tileSizeY, _imp->localTilesState.bounds);


                // Reset the policy to read/write now that we reseted the tiles state map once.
                _imp->cachePolicy = eCacheAccessModeReadWrite;
            }

            // In non peristent mode, the entry pointer is directly cached: there's no call to fromMemorySegment/toMemorySegment
            // Emulate what is done when compiled with a persistent cache
            if (!cacheAccess->isPersistent() && cacheStatus == CacheEntryLockerBase::eCacheEntryStatusCached) {
                _imp->internalCacheEntry = toImageCacheEntryInternal(cacheAccess->getProcessLocalEntry());

                ImageCacheEntryInternal<false>* nonPersistentLocalEntry = dynamic_cast<ImageCacheEntryInternal<false>* >(_imp->internalCacheEntry.get());
                assert(nonPersistentLocalEntry);

                // First call readAndUpdateStateMap under a read lock, if needed recall under a write lock
                bool mustCallUnderWriteLock = false;
                {
                    boost::shared_lock<boost::shared_mutex> readLock(nonPersistentLocalEntry->perMipMapTilesStateMutex);
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
                        case ImageCacheEntryPrivate::eUpdateStateMapRetCodeFailed:
                            return eActionStatusFailed;

                    }

                }
                if (mustCallUnderWriteLock) {
                    boost::unique_lock<boost::shared_mutex> writeLock(nonPersistentLocalEntry->perMipMapTilesStateMutex);
                    ImageCacheEntryPrivate::UpdateStateMapRetCodeEnum stat = _imp->readAndUpdateStateMap(true /*hasExlcusiveLock*/);
                    assert(stat == ImageCacheEntryPrivate::eUpdateStateMapRetCodeMustWriteToCache || stat == ImageCacheEntryPrivate::eUpdateStateMapRetCodeFailed);
                    if (stat == ImageCacheEntryPrivate::eUpdateStateMapRetCodeFailed) {
                        return eActionStatusFailed;
                    }
                }
            }

            // In persistent mode:
            // If the status is CacheEntryLocker::eCacheEntryStatusCached the state map updating was done in fromMemorySegment
            // which was called inside getFromCache()
            // If the status is CacheEntryLocker::eCacheEntryStatusMustCompute:
            // this is the first time we create the ImageCacheEntryInternal in the cache and thus we are the only thread/process here.
            // fromMemorySegment was not called, and the state map is marked with eTileStatusNotRendered everywhere.

            // In non persistent mode we have to do the same thing when it is the first time we create the entry in the cache.

            // The exclusive lock is guaranteed by the cache, we don't have to do anything externally.
            if (cacheStatus == CacheEntryLockerBase::eCacheEntryStatusMustCompute) {

                boost::scoped_ptr<boost::unique_lock<boost::shared_mutex> > writeLock;
                if (!_imp->internalCacheEntry->isPersistent()) {
                    // In non-persistent mode, lock the cache entry since it's shared across threads.
                    // In persistent mode the entry is copied in fromMemorySegment
                    ImageCacheEntryInternal<false>* nonPersistentLocalEntry = dynamic_cast<ImageCacheEntryInternal<false>* >(_imp->internalCacheEntry.get());
                    assert(nonPersistentLocalEntry);
                    writeLock.reset(new boost::unique_lock<boost::shared_mutex>(nonPersistentLocalEntry->perMipMapTilesStateMutex));
                }

                ImageCacheEntryPrivate::UpdateStateMapRetCodeEnum stat = _imp->readAndUpdateStateMap(true /*hasExclusiveLock*/);

                // All tiles should be eTileStatusNotRendered and thus we set them all to eTileStatusPending and must insert the results in te cache
                assert(stat == ImageCacheEntryPrivate::eUpdateStateMapRetCodeMustWriteToCache);
                (void)stat;
                cacheAccess->insertInCache();
            }


            bool markedTilesModified = false;
            for (std::size_t i = 0; i < markedTilesSize.size(); ++i) {
                if (i < _imp->markedTiles.size() && _imp->markedTiles[i].size() != markedTilesSize[i]) {
                    markedTilesModified = true;
                    break;
                }
            }

            if (markedTilesModified || !_imp->tilesToFetch.empty()) {
                boost::scoped_ptr<boost::unique_lock<boost::shared_mutex> > writeLock;
                if (!_imp->internalCacheEntry->isPersistent()) {
                    // In non-persistent mode, lock the cache entry since it's shared across threads.
                    // In persistent mode the entry is copied in fromMemorySegment
                    ImageCacheEntryInternal<false>* nonPersistentLocalEntry = dynamic_cast<ImageCacheEntryInternal<false>* >(_imp->internalCacheEntry.get());
                    assert(nonPersistentLocalEntry);
                    writeLock.reset(new boost::unique_lock<boost::shared_mutex>(nonPersistentLocalEntry->perMipMapTilesStateMutex));
                }

                // If we found any new cached tile, fetch and copy them to our local storage
                ActionRetCodeEnum stat = _imp->fetchAndCopyCachedTiles();
                if (isFailureRetCode(stat)) {
                    return stat;
                }
                
            }
        } // _imp->cachePolicy = eCacheAccessModeNone

    } // locker
    
    getStatus(tileStatus, hasUnRenderedTile, hasPendingResults);
    return eActionStatusOK;
} // fetchCachedTilesAndUpdateStatus

void
ImageCacheEntry::getStatus(TileStateHeader* tileStatus, bool* hasUnRenderedTile, bool *hasPendingResults) const
{

    boost::unique_lock<boost::mutex> locker(_imp->lock);
    if (tileStatus) {
        *tileStatus = TileStateHeader(_imp->localTilesState.tileSizeX, _imp->localTilesState.tileSizeY, _imp->localTilesState.bounds, _imp->localTilesState.state);
    }
    assert((!hasPendingResults && !hasUnRenderedTile) || (hasPendingResults && hasUnRenderedTile));

    if (!hasPendingResults && !hasUnRenderedTile) {
        return;
    }

    *hasPendingResults = _imp->hasPendingTiles;
    if (_imp->mipMapLevel < _imp->markedTiles.size()) {
        *hasUnRenderedTile = _imp->markedTiles[_imp->mipMapLevel].size() > 0;
    } else {
        *hasUnRenderedTile = false;
    }
} // getStatus

void
ImageCacheEntryPrivate::updateCachedTilesStateMap()
{
    assert(cachePolicy != eCacheAccessModeNone);
    if (!internalCacheEntry->isPersistent()) {
        return;
    }

    ImageCacheEntryInternal<true>* persistentLocalEntry = dynamic_cast<ImageCacheEntryInternal<true>* >(internalCacheEntry.get());
    assert(persistentLocalEntry);

    // Re-lookup in the cache the entry, this will force a call to fromMemorySegment, thus updating the tiles state
    // as seen in the cache.
    // There may be a possibility that the tiles state were removed from the cache for some external reasons.
    // In this case we just re-insert back the entry in the cache.

    // Flag that we don't want to update the tile state locally
    persistentLocalEntry->nextFromMemorySegmentCallIsToMemorySegment = true;
    CacheEntryLockerBasePtr cacheAccess = internalCacheEntry->getFromCache();
    CacheEntryLockerBase::CacheEntryStatusEnum cacheStatus = cacheAccess->getStatus();
    while (cacheStatus == CacheEntryLockerBase::eCacheEntryStatusComputationPending) {
        cacheStatus = cacheAccess->waitForPendingEntry();
    }
    assert(cacheStatus == CacheEntryLockerBase::eCacheEntryStatusCached ||
           cacheStatus == CacheEntryLockerBase::eCacheEntryStatusMustCompute);

    // There may be a case where the entry was removed, insert in the cache, forcing a call to toMemorySegment
    if (cacheStatus == CacheEntryLockerBase::eCacheEntryStatusMustCompute) {
        cacheAccess->insertInCache();
    }

    persistentLocalEntry->nextFromMemorySegmentCallIsToMemorySegment = false;
}

void
ImageCacheEntry::markCacheTilesAsAborted()
{
    // Make sure to call fetchCachedTilesAndUpdateStatus() first
    assert(_imp->internalCacheEntry);

    // Protect all local structures against multiple threads using this object.
    boost::unique_lock<boost::mutex> locker(_imp->lock);

    if (_imp->markedTiles.empty()) {
        return;
    }


    boost::scoped_ptr<boost::unique_lock<boost::shared_mutex> > writeLock;
    if (!_imp->internalCacheEntry->isPersistent()) {
        // In non-persistent mode, lock the cache entry since it's shared across threads.
        // In persistent mode the entry is copied in fromMemorySegment
        ImageCacheEntryInternal<false>* nonPersistentLocalEntry = dynamic_cast<ImageCacheEntryInternal<false>* >(_imp->internalCacheEntry.get());
        assert(nonPersistentLocalEntry);
        writeLock.reset(new boost::unique_lock<boost::shared_mutex>(nonPersistentLocalEntry->perMipMapTilesStateMutex));
    }

    // We should have gotten the state map from the cache in fetchCachedTilesAndUpdateStatus()
    assert(!_imp->internalCacheEntry->perMipMapTilesState.empty());


    // Read the cache map and update our local map
    CacheBasePtr cache = _imp->internalCacheEntry->getCache();
    bool hasModifiedTileMap = false;

    RectI mipmap0Bounds = _imp->localTilesState.bounds.upscalePowerOfTwo(_imp->mipMapLevel);
    for (std::size_t i = 0; i < _imp->markedTiles.size(); ++i) {

        RectI levelBounds = mipmap0Bounds.downscalePowerOfTwo(i);
        TileStateHeader cacheStateMap = TileStateHeader(_imp->localTilesState.tileSizeX, _imp->localTilesState.tileSizeY, levelBounds, &_imp->internalCacheEntry->perMipMapTilesState[i]);
        assert(!cacheStateMap.state->tiles.empty());

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

    if (_imp->cachePolicy != eCacheAccessModeNone) {
        // In persistent mode we have to actually copy the cache entry tiles state map to the cache
        if (_imp->internalCacheEntry->isPersistent()) {
            _imp->updateCachedTilesStateMap();
        }

    }

} // markCacheTilesAsAborted

void
ImageCacheEntry::markCacheTilesInRegionAsNotRendered(const RectI& roi)
{
    // Make sure to call fetchCachedTilesAndUpdateStatus() first
    assert(_imp->internalCacheEntry);

    // Protect all local structures against multiple threads using this object.
    boost::unique_lock<boost::mutex> locker(_imp->lock);


    boost::scoped_ptr<boost::unique_lock<boost::shared_mutex> > writeLock;
    if (!_imp->internalCacheEntry->isPersistent()) {
        // In non-persistent mode, lock the cache entry since it's shared across threads.
        // In persistent mode the entry is copied in fromMemorySegment
        ImageCacheEntryInternal<false>* nonPersistentLocalEntry = dynamic_cast<ImageCacheEntryInternal<false>* >(_imp->internalCacheEntry.get());
        assert(nonPersistentLocalEntry);
        writeLock.reset(new boost::unique_lock<boost::shared_mutex>(nonPersistentLocalEntry->perMipMapTilesStateMutex));
    }

    // We should have gotten the state map from the cache in fetchCachedTilesAndUpdateStatus()
    assert(!_imp->internalCacheEntry->perMipMapTilesState.empty());

    // Read the cache map and update our local map
    CacheBasePtr cache = _imp->internalCacheEntry->getCache();
    bool hasModifiedTileMap = false;

    std::vector<U64> localTileIndicesToRelease, cacheTileIndicesToRelease;

    RectI mipmap0Roi = roi.upscalePowerOfTwo(_imp->mipMapLevel);
    RectI mipmap0Bounds = _imp->localTilesState.bounds.upscalePowerOfTwo(_imp->mipMapLevel);
    for (std::size_t i = 0; i < _imp->internalCacheEntry->perMipMapTilesState.size(); ++i) {
        
        RectI levelBounds = mipmap0Bounds.downscalePowerOfTwo(i);
        RectI levelRoi = mipmap0Roi.downscalePowerOfTwo(i);

        RectI roiRounded = levelRoi;
        roiRounded.roundToTileSize(_imp->localTilesState.tileSizeX, _imp->localTilesState.tileSizeY);

        TileStateHeader cacheStateMap = TileStateHeader(_imp->localTilesState.tileSizeX, _imp->localTilesState.tileSizeY, levelBounds, &_imp->internalCacheEntry->perMipMapTilesState[i]);
        assert(!cacheStateMap.state->tiles.empty());



        for (int ty = roiRounded.y1; ty < roiRounded.y2; ty += _imp->localTilesState.tileSizeY) {
            for (int tx = roiRounded.x1; tx < roiRounded.x2; tx += _imp->localTilesState.tileSizeX) {

                assert(tx % _imp->localTilesState.tileSizeX == 0 && ty % _imp->localTilesState.tileSizeY == 0);

                if (i == _imp->mipMapLevel) {
                    TileState* localTileState = _imp->localTilesState.getTileAt(tx, ty);
                    assert(localTileState);
                    localTileState->status = eTileStatusNotRendered;
                }

                TileState* cacheTileState = cacheStateMap.getTileAt(tx, ty);
                assert(cacheTileState);
                cacheTileState->status = eTileStatusNotRendered;
                hasModifiedTileMap = true;
#ifdef TRACE_TILES_STATUS
                qDebug() << QThread::currentThread() << _imp->effect->getScriptName_mt_safe().c_str() << _imp->internalCacheEntry->getHashKey() << "marking " << tx << ty << "unrendered at level" << i;
#endif

                for (int c = 0; c < 4; ++c) {
                    if (cacheTileState->channelsTileStorageIndex[c] != (U64)-1) {
                        cacheTileIndicesToRelease.push_back(cacheTileState->channelsTileStorageIndex[c]);
                        localTileIndicesToRelease.push_back(tx + ty + c + i);
                    }
                }
            }
        }
    }

    if (!cacheTileIndicesToRelease.empty()) {
        _imp->internalCacheEntry->getCache()->releaseTiles(_imp->internalCacheEntry, localTileIndicesToRelease, cacheTileIndicesToRelease);
    }


    if (_imp->cachePolicy != eCacheAccessModeNone && hasModifiedTileMap) {
        // In persistent mode we have to actually copy the cache entry tiles state map to the cache
        if (_imp->internalCacheEntry->isPersistent()) {
            _imp->updateCachedTilesStateMap();
        }
    }

} // markCacheTilesInRegionAsNotRendered

void
ImageCacheEntry::markCacheTilesAsRendered()
{
    // Make sure to call fetchCachedTilesAndUpdateStatus() first
    assert(_imp->internalCacheEntry);

    // Protect all local structures against multiple threads using this object.
    boost::unique_lock<boost::mutex> locker(_imp->lock);
    
    if (_imp->markedTiles.empty()) {
        return;
    }

    boost::scoped_ptr<boost::unique_lock<boost::shared_mutex> > writeLock;
    if (!_imp->internalCacheEntry->isPersistent()) {
        // In non-persistent mode, lock the cache entry since it's shared across threads.
        // In persistent mode the entry is copied in fromMemorySegment
        ImageCacheEntryInternal<false>* nonPersistentLocalEntry = dynamic_cast<ImageCacheEntryInternal<false>* >(_imp->internalCacheEntry.get());
        assert(nonPersistentLocalEntry);
        writeLock.reset(new boost::unique_lock<boost::shared_mutex>(nonPersistentLocalEntry->perMipMapTilesStateMutex));
    }

    // We should have gotten the state map from the cache in fetchCachedTilesAndUpdateStatus()
    assert(!_imp->internalCacheEntry->perMipMapTilesState.empty());

    assert(!_imp->localTilesState.state->tiles.empty());


    // Read the cache map and update our local map
    CacheBasePtr cache = _imp->internalCacheEntry->getCache();
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

                    if (_imp->cachePolicy != eCacheAccessModeNone) {
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
    }
    
    _imp->markedTiles.clear();

#ifdef DEBUG
    // Check that all tiles are marked either rendered or pending
    RectI roiRounded = _imp->roi;
    roiRounded.roundToTileSize(_imp->localTilesState.tileSizeX, _imp->localTilesState.tileSizeY);
    for (int ty = roiRounded.y1; ty < roiRounded.y2; ty += _imp->localTilesState.tileSizeY) {
        for (int tx = roiRounded.x1; tx < roiRounded.x2; tx += _imp->localTilesState.tileSizeX) {

            assert(tx % _imp->localTilesState.tileSizeX == 0 && ty % _imp->localTilesState.tileSizeY == 0);
            TileState* localTileState = _imp->localTilesState.getTileAt(tx, ty);
            assert(localTileState->status == eTileStatusPending || localTileState->status == eTileStatusRendered);
        }
    }
#endif

    if (!hasModifiedTileMap) {
        return;
    }

    // The following is only done when interacting with the cache: we copy our local buffers to the cache
    if (_imp->cachePolicy == eCacheAccessModeNone) {
        return;
    }

    // We are going to fetch data from the cache, ensure our local buffers are allocated
    _imp->image.lock()->ensureBuffersAllocated();


    std::vector<U64> tilesAllocNeeded(tilesToCopy.size());
    for (std::size_t i = 0; i < tilesToCopy.size(); ++i) {
        tilesAllocNeeded[i] = tilesToCopy[i]->bounds.x1 + tilesToCopy[i]->bounds.x2 + tilesToCopy[i]->channel_i + _imp->mipMapLevel;
    }

    // Allocated buffers for tiles
    std::vector<std::pair<U64, void*> > allocatedTiles;
    void* cacheData;
    bool gotTiles = cache->retrieveAndLockTiles(_imp->internalCacheEntry, 0 /*existingTiles*/, &tilesAllocNeeded, NULL, &allocatedTiles, &cacheData);
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
    if (_imp->internalCacheEntry->isPersistent()) {
        _imp->updateCachedTilesStateMap();
    }
} // markCacheTilesAsRendered

bool
ImageCacheEntry::waitForPendingTiles()
{

    {
        boost::unique_lock<boost::mutex> locker(_imp->lock);
        if (!_imp->hasPendingTiles) {
            return true;
        }
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
            CacheEntryLockerBase::sleep_milliseconds(timeToWaitMS);

            // Increase the time to wait at the next iteration
            timeToWaitMS *= 1.2;


        }

    } while(hasPendingResults && !hasUnrenderedTile && !_imp->effect->isRenderAborted());

    if (hasReleasedThread) {
        QThreadPool::globalInstance()->reserveThread();
    }
    return !hasPendingResults && !hasUnrenderedTile;

} // waitForPendingTiles

static std::string getPropNameInternal(const std::string& baseName, unsigned int mipMapLevel)
{
    std::stringstream ss;
    ss << baseName << mipMapLevel;
    return ss.str();
}

static std::string getStatusPropName(unsigned int mipMapLevel)
{
    return getPropNameInternal("Status", mipMapLevel);
}

static std::string getTileIndicesPropName(unsigned int mipMapLevel)
{
    return getPropNameInternal("TileIndices", mipMapLevel);
}

static std::string getBoundsPropName(unsigned int mipMapLevel)
{
    return getPropNameInternal("Bounds", mipMapLevel);
}

static CacheEntryBase::FromMemorySegmentRetCodeEnum fromMemorySegmentInternal(const IPCPropertyMap& properties, std::vector<TilesState>* localMipMapStates)
{

    int numLevels;
    if (!properties.getIPCProperty("NumLevels", 0, &numLevels)) {
        return CacheEntryBase::eFromMemorySegmentRetCodeFailed;
    }
    localMipMapStates->resize(numLevels);

    for (std::size_t m = 0; m < localMipMapStates->size(); ++m) {

        TilesState& localState = (*localMipMapStates)[m];

        std::string statusPropName = getStatusPropName(m);
        std::string tileIndicesPropName = getTileIndicesPropName(m);
        std::string boundsPropName = getBoundsPropName(m);

        const IPCProperty* statusProp = properties.getIPCProperty(statusPropName);
        const IPCProperty* indicesProp = properties.getIPCProperty(tileIndicesPropName);
        const IPCProperty* boundsProp = properties.getIPCProperty(boundsPropName);

        if (!statusProp || statusProp->getType() != eIPCVariantTypeInt) {
            return CacheEntryBase::eFromMemorySegmentRetCodeFailed;
        }

        if (!indicesProp || indicesProp->getType() != eIPCVariantTypeULongLong) {
            return CacheEntryBase::eFromMemorySegmentRetCodeFailed;
        }

        if (!boundsProp || boundsProp->getType() != eIPCVariantTypeInt) {
            return CacheEntryBase::eFromMemorySegmentRetCodeFailed;
        }

        if (indicesProp->getNumDimensions() != statusProp->getNumDimensions() * 4 ||
            boundsProp->getNumDimensions() != statusProp->getNumDimensions() * 4) {
            return CacheEntryBase::eFromMemorySegmentRetCodeFailed;
        }

        localState.tiles.resize(statusProp->getNumDimensions());

        for (std::size_t i = 0; i < localState.tiles.size(); ++i) {
            TileState& state = localState.tiles[i];

            int status_i;
            IPCProperty::getIntValue(statusProp->getData(), i, &status_i);
            state.status = (TileStatusEnum)status_i;

            for (int c = 0; c < 4; ++c) {
                IPCProperty::getULongLongValue(indicesProp->getData(), i * 4 + c, &state.channelsTileStorageIndex[c]);
            }

            IPCProperty::getIntValue(boundsProp->getData(), i * 4, &state.bounds.x1);
            IPCProperty::getIntValue(boundsProp->getData(), i * 4 + 1, &state.bounds.y1);
            IPCProperty::getIntValue(boundsProp->getData(), i * 4 + 2, &state.bounds.x2);
            IPCProperty::getIntValue(boundsProp->getData(), i * 4 + 3, &state.bounds.y2);

        }
    }
    return CacheEntryBase::eFromMemorySegmentRetCodeOk;
}

/**
 * @brief The implementation that copies the tile status from the local state map to the cache state map
 * @param copyPendingStatusToCache If true then we blindly copy the status to the cache. If false, we only
 * update tiles that are locally marked rendered or not rendered.
 **/
static void toMemorySegmentInternal(bool copyPendingStatusToCache,
                                    const std::vector<TilesState>& localMipMapStates,
                                    IPCPropertyMap* properties)
{



    int numLevels = (int)localMipMapStates.size();
    properties->setIPCProperty("NumLevels", numLevels);

    for (std::size_t m = 0; m < localMipMapStates.size(); ++m) {

        const TilesState& mipmapState = localMipMapStates[m];

        std::string statusPropName = getStatusPropName(m);
        std::string tileIndicesPropName = getTileIndicesPropName(m);
        std::string boundsPropName = getBoundsPropName(m);

        IPCProperty* statusProp = properties->getOrCreateIPCProperty(statusPropName, eIPCVariantTypeInt);
        IPCProperty* indicesProp = properties->getOrCreateIPCProperty(tileIndicesPropName, eIPCVariantTypeULongLong);
        IPCProperty* boundsProp = properties->getOrCreateIPCProperty(boundsPropName, eIPCVariantTypeInt);

        if (statusProp->getNumDimensions() == 0 || copyPendingStatusToCache) {

            statusProp->resize(mipmapState.tiles.size());

            // Each tile has up to 4 channels allocated in the cache
            indicesProp->resize(mipmapState.tiles.size() * 4);

            // Each tile bounds is 4 double
            boundsProp->resize(mipmapState.tiles.size() * 4);

            for (std::size_t i = 0; i < mipmapState.tiles.size(); ++i) {
                IPCProperty::setIntValue(i, (int)mipmapState.tiles[i].status, statusProp->getData());
                for (int c = 0; c < 4; ++c) {
                    IPCProperty::setULongLongValue(i * 4 + c, mipmapState.tiles[i].channelsTileStorageIndex[c], indicesProp->getData());
                }

                IPCProperty::setIntValue(i * 4, mipmapState.tiles[i].bounds.x1, boundsProp->getData());
                IPCProperty::setIntValue(i * 4 + 1, mipmapState.tiles[i].bounds.y1, boundsProp->getData());
                IPCProperty::setIntValue(i * 4 + 2, mipmapState.tiles[i].bounds.x2, boundsProp->getData());
                IPCProperty::setIntValue(i * 4 + 3, mipmapState.tiles[i].bounds.y2, boundsProp->getData());

            }

        } else {
            assert(statusProp->getNumDimensions() == mipmapState.tiles.size() &&
                   indicesProp->getNumDimensions() == mipmapState.tiles.size() * 4 &&
                   boundsProp->getNumDimensions() == mipmapState.tiles.size() * 4);


            for (std::size_t i = 0; i < mipmapState.tiles.size(); ++i) {

                // Do not write over rendered tiles
                {
                    int cacheStatus_i;
                    IPCProperty::getIntValue(*statusProp->getData(), i, &cacheStatus_i);
                    if ((TileStatusEnum)cacheStatus_i == eTileStatusRendered) {
                        continue;
                    }
                }

                if (mipmapState.tiles[i].status == eTileStatusNotRendered || mipmapState.tiles[i].status == eTileStatusRendered) {

                    IPCProperty::setIntValue(i, (int)mipmapState.tiles[i].status, statusProp->getData());
                    for (int c = 0; c < 4; ++c) {
                        IPCProperty::setULongLongValue(i * 4 + c, mipmapState.tiles[i].channelsTileStorageIndex[c], indicesProp->getData());
                    }

                    IPCProperty::setIntValue(i * 4, mipmapState.tiles[i].bounds.x1, boundsProp->getData());
                    IPCProperty::setIntValue(i * 4 + 1, mipmapState.tiles[i].bounds.y1, boundsProp->getData());
                    IPCProperty::setIntValue(i * 4 + 2, mipmapState.tiles[i].bounds.x2, boundsProp->getData());
                    IPCProperty::setIntValue(i * 4 + 3, mipmapState.tiles[i].bounds.y2, boundsProp->getData());

                }
                

            }
            
        }
    } // For each mipmap level
}

CacheEntryBase::FromMemorySegmentRetCodeEnum
ImageCacheEntryInternalBase::fromMemorySegment(bool isLockedForWriting,
                                           const IPCPropertyMap& properties) {

    if (!isPersistent()) {
        throw std::runtime_error("ImageCacheEntryInternal::fromMemorySegment called on a non persistent cache!");
    }

    ImageCacheEntryInternal<true>* persistentEntry = dynamic_cast<ImageCacheEntryInternal<true>*>(this);

    // Deserialize and optionnally update the tiles state
    {


        // Read our tiles state vector

        if (persistentEntry->nextFromMemorySegmentCallIsToMemorySegment) {
            // We have written the state map from within markCacheTilesAsRendered(), update it if possible
            if (!isLockedForWriting) {
                // We must update the state map but cannot do so under a read lock
                return CacheEntryBase::eFromMemorySegmentRetCodeNeedWriteLock;
            }

            // If we are here, we are either in markCacheTilesAsAborted() or markCacheTilesAsRendered().
            // We may have read pending tiles from the cache that are no longer pending in the cache and we don't want to write over it
            // hence we do not update tiles that are marked pending
            IPCPropertyMap* hackedProps = const_cast<IPCPropertyMap*>(&properties);
            toMemorySegmentInternal(false /*copyPendingStatusToCache*/, perMipMapTilesState, hackedProps);
        } else {

            // Copy the tiles states locally
            fromMemorySegmentInternal(properties, &perMipMapTilesState);

            // Read the state map: it might modify it by marking tiles pending
            ImageCacheEntryPrivate::UpdateStateMapRetCodeEnum stat = persistentEntry->_imp->readAndUpdateStateMap(isLockedForWriting);
            switch (stat) {
                case ImageCacheEntryPrivate::eUpdateStateMapRetCodeUpToDate:
                    // We did not write anything on the state map, we are good
                    break;
                case ImageCacheEntryPrivate::eUpdateStateMapRetCodeMustWriteToCache: {
                    // We have written the state map, we must update it
                    assert(isLockedForWriting);
                    IPCPropertyMap* hackedProps = const_cast<IPCPropertyMap*>(&properties);
                    toMemorySegmentInternal(true /*copyPendingStatusToCache*/, perMipMapTilesState, hackedProps);

                }   break;
                case ImageCacheEntryPrivate::eUpdateStateMapRetCodeNeedWriteLock:
                    // We must update the state map but cannot do so under a read lock
                    return CacheEntryBase::eFromMemorySegmentRetCodeNeedWriteLock;
                case ImageCacheEntryPrivate::eUpdateStateMapRetCodeFailed:
                    return CacheEntryBase::eFromMemorySegmentRetCodeFailed;
            }
        }
    }
    
    return CacheEntryBase::fromMemorySegment(isLockedForWriting, properties);
} // fromMemorySegment


void
ImageCacheEntryInternalBase::toMemorySegment(IPCPropertyMap* properties) const
{

    if (!isPersistent()) {
        throw std::runtime_error("ImageCacheEntryInternal::toMemorySegment called on a non persistent cache!");
    }
    // Copy the tile state to the memory segment, this will be called the first time we construct the entry in the cache
    toMemorySegmentInternal(true /*copyPendingStatusToCache*/, perMipMapTilesState, properties);
    CacheEntryBase::toMemorySegment(properties);
} // toMemorySegment


NATRON_NAMESPACE_EXIT;
