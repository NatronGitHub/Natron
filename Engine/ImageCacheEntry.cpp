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


struct TileCacheIndex
{
    // Indices of the tiles in the cache for each channel
    int perChannelTileIndices[4];

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
    PerMipMapTileStateVector perMipMapTilesState;

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
    int tileCache_i;
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

    // State indicating the render status of each tiles at the mipMapLevel
    // The state is shared accross all channels because OpenFX doesn't allows yet to render only some channels
    // This state is local to this ImageCacheEntry: any tile marked eTileStatusPending may be pending because
    // we are computing it or someone else is computing it.
    TileStateMap state;

    // Pointer to the internal cache entry. When NATRON_CACHE_NEVER_PERSISTENT is defined, this is shared across threads/processes
    // otherwise this is local to this object.
    ImageCacheEntryInternalPtr internalCacheEntry;

    // When reading the state map, this is the list of tiles to fetch from the cache in fetchAndCopyCachedTiles()
    std::vector<TileCacheIndex> tilesToFetch;

    

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
        state.init(tileSizeX, tileSizeY, pixelRod);
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
     * Note that this function does the pixels transfer
     **/
    bool fetchAndCopyCachedTiles();

#ifndef NATRON_CACHE_NEVER_PERSISTENT
    void updateCachedTilesStateMap();
#endif


    TileStatusEnum lookupTileStateInPyramid(const std::vector<TileStateMap>& perMipMapTilesState,
                                            unsigned int mipMapLevel,
                                            int tx, int ty,
                                            TileCacheIndex* tile);

    std::vector<boost::shared_ptr<TileData> > buildTaskPyramidRecursive(unsigned int lookupLevel,
                                                                        const TileCacheIndex &tile,
                                                                        const std::vector<void*> &fetchedExistingTiles,
                                                                        const std::vector<std::pair<int, void*> >& allocatedTiles,
                                                                        int* existingTiles_i,
                                                                        int* allocatedTiles_i,
                                                                        std::vector<boost::shared_ptr<TileData> > *tilesToCopy,
                                                                        std::vector<std::vector<boost::shared_ptr<DownscaleTile> > >  *downscaleTilesPerLevel);

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

    std::vector<boost::shared_ptr<TileData> > _tasks;
    Image::CPUData _localBuffer;

public:

    CachePixelsTransferProcessor(const EffectInstancePtr& renderClone)
    : MultiThreadProcessorBase(renderClone)
    , _tasks()
    , _localBuffer()
    {

    }

    virtual ~CachePixelsTransferProcessor()
    {
    }

    void setValues(const Image::CPUData& localBuffer,
                   const std::vector<boost::shared_ptr<TileData> >& tasks)
    {
        _localBuffer = localBuffer;
        _tasks = tasks;
    }


    virtual ActionRetCodeEnum multiThreadFunction(unsigned int threadID,
                                                  unsigned int nThreads) OVERRIDE FINAL WARN_UNUSED_RETURN
    {

        int fromIndex, toIndex;
        ImageMultiThreadProcessorBase::getThreadRange(threadID, nThreads, 0, _tasks.size(), &fromIndex, &toIndex);
        for (int i = fromIndex; i < toIndex; ++i) {

            const TileData& task = *_tasks[i];

            RectI renderWindow = _localBuffer.bounds;
            task.bounds.intersect(_localBuffer.bounds, &renderWindow);

            if (copyToCache) {
                copyPixels(renderWindow, _localBuffer.bitDepth, _localBuffer.ptrs[task.channel_i], _localBuffer.nComps, _localBuffer.bounds.width() * _localBuffer.nComps, task.ptr, 1, task.bounds.width());
            } else {
                copyPixels(renderWindow, _localBuffer.bitDepth, task.ptr, 1, task.bounds.width(), _localBuffer.ptrs[task.channel_i], _localBuffer.nComps, _localBuffer.bounds.width() * _localBuffer.nComps);
            }

        }
        return eActionStatusOK;
    } // multiThreadFunction
};

typedef CachePixelsTransferProcessor<true> ToCachePixelsTransferProcessor;
typedef CachePixelsTransferProcessor<false> FromCachePixelsTransferProcessor;

template <typename PIX>
PIX* getPix(PIX* ptr,
            int x, int y,
            const RectI& bounds)
{
    if (x < bounds.x1 || x >= bounds.x2 || y < bounds.y1 || y >= bounds.y2) {
        return 0;
    }
    return ptr + bounds.width() * y + x;
}

template <typename PIX>
static void fillWithConstant(PIX val,
                             PIX* ptr,
                             const RectI& roi,
                             const RectI& bounds)
{

    PIX* pix = getPix(ptr, roi.x1, roi.y1);
    for (int y = roi.y1; y < roi.y2; ++y) {
        for (int x = roi.x1; x < roi.x2; ++x) {
            *pix = val;
            ++pix;
        }
        pix += (bounds.width() - roi.width());
    }
}

/**
 * @brief Returns true if the rectangle "bounds" is aligned to the tile size or not
 **/
static bool isTileAligned(const RectI& bounds,
                          int tileSizeX,
                          int tileSizeY)
{
    assert(bounds.x1 % tileSizeX ||
           bounds.x2 % tileSizeX);
    assert(bounds.y1 % tileSizeX ||
           bounds.y2 % tileSizeY);
    assert(bounds.width() <= tileSizeX);
    assert(bounds.height() <= tileSizeY);
    return bounds.x1 % tileSizeX != 0 || bounds.x2 % tileSizeX != 0 || bounds.y1 % tileSizeY != 0 || bounds.y2 % tileSizeY != 0;
}

/**
 * @brief If the tile bounds is not aligned to the tile size, repeat pixels on the edge.
 * This function fills the numbered rectangles with the nearest pixel V as such:

 12223
 4VVV5
 4VVV5
 67778
 **/
template <typename PIX>
static void repeatEdges(PIX* ptr,
                        const RectI& bounds,
                        int tileSizeX,
                        int tileSizeY)
{
    assert(bounds.x1 % tileSizeX ||
           bounds.x2 % tileSizeX);
    assert(bounds.y1 % tileSizeX ||
           bounds.y2 % tileSizeY);
    assert(bounds.width() <= tileSizeX);
    assert(bounds.height() <= tileSizeY);

    RectI roundedBounds = bounds;
    roundedBounds.roundToTileSize(tileSizeX, tileSizeY);

    {
        // 1
        RectI roi(roundedBounds.x1, bounds.y2, bounds.x1, roundedBounds.y2);
        if (!roi.isNull()) {
            PIX val = *getPix(ptr, bounds.x1, bounds.y2 - 1, roundedBounds);
            fillWithConstant(val, ptr, roi, roundedBounds);
        }
    }
    {
        // 2
        RectI roi(bounds.x1, bounds.y2, roundedBounds.x2, roundedBounds.y2);
        if (!roi.isNull()) {

            PIX* origPix = getPix(ptr, bounds.x1, bounds.y2 - 1, roundedBounds);

            PIX* pix = getPix(ptr, roi.x1, roi.y1);
            for (int y = roi.y1; y < roi.y2; ++y) {
                PIX* srcPix = origPix;
                for (int x = roi.x1; x < roi.x2; ++x) {
                    *pix = srcPix;
                    ++pix;
                    ++origPix;
                }
                // Remove what was done in the last iteration and go to the next line
                pix += (roundedBounds.width() - roi.width());
            }
        }
    }
    {
        // 3
        RectI roi(bounds.x2, bounds.y2, roundedBounds.x2, roundedBounds.y2);
        if (!roi.isNull()) {
            PIX val = *getPix(ptr, bounds.x2 - 1, bounds.y2 - 1, roundedBounds);
            fillWithConstant(val, ptr, roi, roundedBounds);
        }
    }
    {
        // 4
        RectI roi(roundedBounds.x1, bounds.y1, bounds.x1, bounds.y2);
        if (!roi.isNull()) {
            PIX* pix = getPix(ptr, roi.x1, roi.y1);
            for (int y = roi.y1; y < roi.y2; ++y) {
                PIX val = *getPix(ptr, bounds.x1, y, roundedBounds);
                for (int x = roi.x1; x < roi.x2; ++x) {
                    *pix = val;
                    ++pix;
                }
                // Remove what was done in the last iteration and go to the next line
                pix += (roundedBounds.width() - roi.width());
            }
        }
    }
    {
        // 5
        RectI roi(bounds.x2, bounds.y1, roundedBounds.x1, bounds.y2);
        if (!roi.isNull()) {
            PIX* pix = getPix(ptr, roi.x1, roi.y1);
            for (int y = roi.y1; y < roi.y2; ++y) {
                PIX val = *getPix(ptr, bounds.x2 - 1, y, roundedBounds);
                for (int x = roi.x1; x < roi.x2; ++x) {
                    *pix = val;
                    ++pix;
                }
                // Remove what was done in the last iteration and go to the next line
                pix += (roundedBounds.width() - roi.width());
            }
        }
    }
    {
        // 6
        RectI roi(roundedBounds.x1, roundedBounds.y1, bounds.x1, bounds.y1);
        if (!roi.isNull()) {
            PIX val = *ptr;
            fillWithConstant(val, ptr, roi, roundedBounds);
        }
    }
    {
        // 7
        RectI roi(bounds.x1, roundedBounds.y1, bounds.x2, bounds.y1);
        if (!roi.isNull()) {

            PIX* origPix = getPix(ptr, bounds.x1, bounds.y1, roundedBounds);

            PIX* pix = getPix(ptr, roi.x1, roi.y1);
            for (int y = roi.y1; y < roi.y2; ++y) {
                PIX* srcPix = origPix;
                for (int x = roi.x1; x < roi.x2; ++x) {
                    *pix = srcPix;
                    ++pix;
                    ++origPix;
                }
                // Remove what was done in the last iteration and go to the next line
                pix += (roundedBounds.width() - roi.width());
            }
        }
    }
    {
        // 8
        RectI roi(bounds.x2, roundedBounds.y1, roundedBounds.x2, bounds.y1);
        if (!roi.isNull()) {
            PIX val = getPix(ptr, bounds.x2 - 1, bounds.y1);
            fillWithConstant(val, ptr, roi, roundedBounds);
        }
    }

} // repeatEdges

template <typename PIX>
static void downscaleMipMapForDepth(const PIX* srcTilesPtr[4],
                                    PIX* dstTilePtr,
                                    const RectI& dstTileBounds,
                                    int tileSizeX,
                                    int tileSizeY)
{

    // Higher scale tiles are indexed as follow:
    //  ---------
    //  | 2 | 3 |
    //  ---------
    //  | 0 | 1 |
    //  ---------

    // All tiles have the same bounds: we don't care about coordinates in this case nor checking whether we are in the bounds

    for (int i = 0; i < 4; ++i) {

        PIX* dst_pixels = dstTilePtr + i * (tileSizeX * tileSizeY / 4);
        const PIX* src_pixels = srcTilesPtr[i];
        const PIX* src_pixels_next = srcTilesPtr[i] + tileSizeX;

        for (int y = 0; y < (int)(tileSizeY / 2); ++y) {
            for (int x = 0; x < (int)(tileSizeX / 2); ++x) {

                double sum = (double)*src_pixels + (double)*(src_pixels + 1);
                sum += ((double)*src_pixels_next + (double)*(src_pixels_next + 1));
                sum /= 4;
                *dst_pixels = (PIX)sum;

                src_pixels_next += 2;
                src_pixels += 2;
                ++dst_pixels;
            }
            src_pixels += tileSizeX;
            src_pixels_next += tileSizeX;
        }
    }

    if (isTileAligned(dstTileBounds, tileSizeX, tileSizeY)) {
        repeatEdges(dstTilePtr, dstTileBounds, tileSizeX, tileSizeY);
    }

} // downscaleMipMapForDepth

static void downscaleMipMap(ImageBitDepthEnum depth,
                            const void* srcTilesPtr[4],
                            void* dstTilePtr,
                            const RectI& dstTileBounds,
                            int tileSizeX,
                            int tileSizeY)
{
    switch (depth) {
        case eImageBitDepthByte:
            downscaleMipMapForDepth<char>((const char**)srcTilesPtr, (char*)dstTilePtr, dstTileBounds, tileSizeX, tileSizeY);
            break;
        case eImageBitDepthShort:
            downscaleMipMapForDepth<unsigned short>((const unsigned short**)srcTilesPtr, (unsigned short*)dstTilePtr, dstTileBounds, tileSizeX, tileSizeY);
            break;
        case eImageBitDepthFloat:
            downscaleMipMapForDepth<float>((const float**)srcTilesPtr, (float*)dstTilePtr, dstTileBounds, tileSizeX, tileSizeY);
            break;
        default:
            break;
    }

}

class DownscaleMipMapProcessor : public MultiThreadProcessorBase
{


private:

    ImageBitDepthEnum _bitdepth;
    std::vector<boost::shared_ptr<DownscaleTile> > _tasks;
    int _tileSizeX, _tileSizeY;

public:

    DownscaleMipMapProcessor(const EffectInstancePtr& renderClone)
    : MultiThreadProcessorBase(renderClone)
    , _bitdepth(eImageBitDepthByte)
    , _tasks()
    , _tileSizeX(-1)
    , _tileSizeY(-1)
    {

    }

    virtual ~DownscaleMipMapProcessor()
    {
    }

    void setValues(ImageBitDepthEnum depth, int tileSizeX, int tileSizeY, const std::vector<boost::shared_ptr<DownscaleTile> >& tasks)
    {
        _bitdepth = depth;
        _tileSizeX = tileSizeX;
        _tileSizeY = tileSizeY;
        _tasks = tasks;
    }


    virtual ActionRetCodeEnum multiThreadFunction(unsigned int threadID,
                                                  unsigned int nThreads) OVERRIDE FINAL WARN_UNUSED_RETURN
    {

        int fromIndex, toIndex;
        ImageMultiThreadProcessorBase::getThreadRange(threadID, nThreads, 0, _tasks.size(), &fromIndex, &toIndex);
        for (int i = fromIndex; i < toIndex; ++i) {

            const DownscaleTile& task = *_tasks[i];

            const void* srcPtrs[4] = {task.srcTiles[0]->ptr, task.srcTiles[1]->ptr, task.srcTiles[2]->ptr, task.srcTiles[3]->ptr};
            downscaleMipMap(_bitdepth, srcPtrs, task.ptr, task.bounds, _tileSizeX, _tileSizeY);
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

TileStatusEnum
ImageCacheEntryPrivate::lookupTileStateInPyramid(const std::vector<TileStateMap>& perMipMapTilesState,
                                                 unsigned int lookupLevel,
                                                 int tx, int ty,
                                                 TileCacheIndex* tile)
{
    const TileStateMap& cachedTilesState = perMipMapTilesState[lookupLevel];
    
    std::vector<TileCacheIndex> tilesToFetch;

    // Find this tile in the current state
    const TileState* tileState = cachedTilesState.getTileAt(tx, ty);

    tile->tx = tx;
    tile->ty = ty;

    switch (tileState->status) {
        case eTileStatusRendered: {
            // If the tile has become rendered, mark it in the tiles to
            // fetch list. This will be fetched later on (when outside of the cache
            // scope) in fetchAndCopyCachedTiles()
            for (int k = 0; k < localData.nComps; ++k) {
                tile->perChannelTileIndices[k] = tileState->channelsTileStorageIndex[k];
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

                int nextTx = tx * 2;
                int nextTy = ty * 2;

                int upscaledX[4] = {nextTx, nextTx + state.tileSizeX, nextTx, nextTx + state.tileSizeX};
                int upscaledY[4] = {nextTy, nextTy, nextTy + state.tileSizeY, nextTy + state.tileSizeY};


                bool higherScaleTilePending = false;
                for (int i = 0; i < 4; ++i) {
                    tile->upscaleTiles[i].reset(new TileCacheIndex);
                    TileStatusEnum higherScaleStatus = lookupTileStateInPyramid(perMipMapTilesState, lookupLevel - 1, upscaledX[i], upscaledY[i], tile->upscaleTiles[i].get());
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

    // Make the map objects from the tile vectors stored in the cache
    std::vector<TileStateMap> perMipMapCacheTilesState(mipMapLevel + 1);
    for (int i = perMipMapCacheTilesState.size() - 1; i >= 0; --i) {
        RectI upscaledBounds = (i == (int)mipMapLevel) ? state.bounds : state.bounds.upscalePowerOfTwo(1);
        perMipMapCacheTilesState[i] = TileStateMap(state.tileSizeX, state.tileSizeY, upscaledBounds, &internalCacheEntry->perMipMapTilesState[i]);
    }

    // If the tiles state at our mipmap level is empty, default initialize it
    bool cachedTilesMapInitialized = !perMipMapCacheTilesState[mipMapLevel].tiles->empty();
    if (!cachedTilesMapInitialized) {
        *perMipMapCacheTilesState[mipMapLevel].tiles = *state.tiles;
        stateMapModified = true;

    } else { // cachedTilesMapInitialized


        // Clear the tiles to fetch list
        tilesToFetch.clear();

        TileStateMap& cacheStateMap = perMipMapCacheTilesState[mipMapLevel];


        // For each tile in the RoI (rounded to the tile size):
        // Check the tile status, only copy from the cache if rendered
        for (int ty = roiRoundedToTileSize.y1; ty < roiRoundedToTileSize.y2; ty += state.tileSizeY) {
            for (int tx = roiRoundedToTileSize.x1; tx < roiRoundedToTileSize.x2; tx += state.tileSizeX) {

                assert(tx % state.tileSizeX == 0 && ty % state.tileSizeY == 0);

                TileState* localTileState = state.getTileAt(tx, ty);

                // If the tile in the old status is already rendered, do not update
                if (localTileState->status == eTileStatusRendered) {
                    continue;
                }


                // Traverse the mipmaps pyramid from lower scale to higher scale to find a rendered tile and then downscale if necessary
                TileCacheIndex tile;
                TileStatusEnum stat = lookupTileStateInPyramid(perMipMapCacheTilesState, mipMapLevel, tx, ty, &tile);


                // Update the status of the tile according to the cache status
                TileState* cacheTileState = cacheStateMap.getTileAt(tx, ty);

                switch (stat) {
                    case eTileStatusRendered: {

                        // If the tile has become rendered, mark it in the tiles to
                        // fetch list. This will be fetched later on (when outside of the cache
                        // scope) in fetchAndCopyCachedTiles()
                        tilesToFetch.push_back(tile);

                        // If this tile is not yet rendered at this level but we found a higher scale mipmap,
                        // we must write this tile as pending in the cache state map so that another thread does not attempt to downscale too
                        if (tile.upscaleTiles[0]) {
                            if (!hasExclusiveLock) {
                                return ImageCacheEntryPrivate::eUpdateStateMapRetCodeNeedWriteLock;
                            }
                            cacheTileState->status = eTileStatusPending;
                            stateMapModified = true;
                            localTileState->status = eTileStatusNotRendered;
                        } else {
                            // Locally, update the status to rendered
                            localTileState->status = eTileStatusRendered;
                        }


                    }   break;
                    case eTileStatusPending: {
                        // If the tile is pending in the cache, leave it pending locally
                        localTileState->status = eTileStatusPending;
                    }   break;
                    case eTileStatusNotRendered: {
                        // If the tile is marked not rendered, mark it pending in the cache
                        // but mark it not rendered for us locally
                        // Only a single thread/process can mark this tile as pending
                        // hence it must do so under a write lock (hasExclusiveLock)
                        if (!hasExclusiveLock) {
                            return ImageCacheEntryPrivate::eUpdateStateMapRetCodeNeedWriteLock;
                        }
                        localTileState->status = eTileStatusNotRendered;
                        cacheTileState->status = eTileStatusPending;
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


/**
 * @brief Lookup for the given tile the tile indices in the cache.
 * If this tile needs to be computed from the average of 4 upscaled tiles, this will also request a tile
 * to be allocated and recursively call the function upstream.
 **/
static void fetchTileIndicesInPyramid(const TileCacheIndex& tile, int nComps, std::vector<int> *tileIndicesToFetch, std::size_t* nTilesAllocNeeded)
{
    if (tile.upscaleTiles[0]) {
        // We must downscale the upscaled tiles
        ++(*nTilesAllocNeeded);
        for (int i = 0; i < 4; ++i) {
            assert(tile.upscaleTiles[i]);
            fetchTileIndicesInPyramid(*tile.upscaleTiles[i], nComps, tileIndicesToFetch, nTilesAllocNeeded);
        }
    } else {
        for (int c = 0; c < nComps; ++c) {
            assert(tile.perChannelTileIndices[c] != -1);
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
                                                  const std::vector<std::pair<int, void*> >& allocatedTiles,
                                                  int* existingTiles_i,
                                                  int* allocatedTiles_i,
                                                  std::vector<boost::shared_ptr<TileData> > *tilesToCopy,
                                                  std::vector<std::vector<boost::shared_ptr<DownscaleTile> > >  *downscaleTilesPerLevel)
{

    std::vector<boost::shared_ptr<TileData> > outputTasks(localData.nComps);


    // The tile bounds are clipped to the pixel RoD
    RectI tileBounds;
    tileBounds.x1 = std::max(tile.tx, state.bounds.x1);
    tileBounds.y1 = std::max(tile.ty, state.bounds.y1);
    tileBounds.x2 = std::min(tile.tx + state.tileSizeX, state.bounds.x2);
    tileBounds.y2 = std::min(tile.ty + state.tileSizeY, state.bounds.y2);

    assert(downscaleTilesPerLevel->size() > lookupLevel);

    if (tile.upscaleTiles[0]) {

        // Recurse upstream.
        // For each 4 upscaled tiles, it returns us a vector of exactly nComps tasks.
        std::vector<boost::shared_ptr<TileData> > upscaledTasks[4];
        for (int i = 0; i < 4; ++i) {
            assert(tile.upscaleTiles[i]);
            upscaledTasks[i] = buildTaskPyramidRecursive(lookupLevel - 1, *tile.upscaleTiles[i],  fetchedExistingTiles, allocatedTiles, existingTiles_i, allocatedTiles_i, tilesToCopy, downscaleTilesPerLevel);

        }
        assert((int)upscaledTasks[0].size() == localData.nComps && (int)upscaledTasks[1].size() == localData.nComps && (int)upscaledTasks[2].size() == localData.nComps && (int)upscaledTasks[3].size() == localData.nComps);

        // Create tasks at this level
        for (int c = 0; c < localData.nComps; ++c) {
            boost::shared_ptr<DownscaleTile> task(new DownscaleTile);
            task->bounds = tileBounds;
            assert(*allocatedTiles_i >= 0 && *allocatedTiles_i < (int)allocatedTiles.size());
            task->ptr = allocatedTiles[*allocatedTiles_i].second;
            task->tileCache_i = allocatedTiles[*allocatedTiles_i].first;
            task->channel_i = c;
            ++(*allocatedTiles_i);
            for (int i = 0; i < 4; ++i) {
                task->srcTiles[i] = upscaledTasks[i][c];
            }

            (*downscaleTilesPerLevel)[lookupLevel].push_back(task);
        }
        
    } else { // !tile.upscaleTiles[0]
        for (int c = 0; c < localData.nComps; ++c) {
            assert(tile.perChannelTileIndices[c] != -1);


            outputTasks[c].reset(new TileData);
            assert(*existingTiles_i >= 0 && *existingTiles_i < (int)fetchedExistingTiles.size());
            outputTasks[c]->ptr = fetchedExistingTiles[*existingTiles_i];
            ++(*existingTiles_i);
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

bool
ImageCacheEntryPrivate::fetchAndCopyCachedTiles()
{

    // Get a vector of tile indices to fetch from the cache directly
    std::vector<int> tileIndicesToFetch;

    // Number of tiles to allocate to downscale
    std::size_t nTilesAllocNeeded = 0;
    for (std::size_t i = 0; i < tilesToFetch.size(); ++i) {
        fetchTileIndicesInPyramid(tilesToFetch[i], localData.nComps, &tileIndicesToFetch, &nTilesAllocNeeded);
    }

    if (tileIndicesToFetch.empty() && nTilesAllocNeeded) {
        // Nothing to do
        return true;
    }

    // Get the tile pointers on the cache
    CachePtr tileCache = internalCacheEntry->getCache();

    // List of the tiles we need to copy
    std::vector<void*> fetchedExistingTiles;

    // Allocated buffers for tiles
    std::vector<std::pair<int, void*> > allocatedTiles;
    void* cacheData;
    bool gotTiles = tileCache->retrieveAndLockTiles(internalCacheEntry, &tileIndicesToFetch, nTilesAllocNeeded, &fetchedExistingTiles, &allocatedTiles, &cacheData);
    CacheDataLock_RAII cacheDataDeleter(tileCache, cacheData);
    if (!gotTiles) {
        return false;
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
    for (std::size_t i = 0; i < perLevelTilesToDownscale.size(); ++i) {
        if (!perLevelTilesToDownscale[i].empty()) {
            DownscaleMipMapProcessor processor(effect);
            processor.setValues(localData.bitDepth, state.tileSizeX, state.tileSizeY, perLevelTilesToDownscale[i]);
            ActionRetCodeEnum stat = processor.launchThreadsBlocking();
            (void)stat;

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
                int tx = (int)std::floor((double)tileBounds.x1 / state.tileSizeX) * state.tileSizeX;
                int ty = (int)std::floor((double)tileBounds.y1 / state.tileSizeY) * state.tileSizeY;

                // Update the state locally
                TileState* tileState = state.getTileAt(tx, ty);
                tileState->channelsTileStorageIndex[perLevelTilesToDownscale[i][j]->channel_i] = perLevelTilesToDownscale[i][j]->tileCache_i;


                TileState* cacheTileState = state.getTileAt(tx, ty);
                cacheTileState->channelsTileStorageIndex[perLevelTilesToDownscale[i][j]->channel_i] = perLevelTilesToDownscale[i][j]->tileCache_i;

                // Update the tile state only for the first channel
                if (perLevelTilesToDownscale[i][j]->channel_i != 0) {
                    continue;
                }

                assert(tileState->status == eTileStatusNotRendered);
                tileState->status = eTileStatusRendered;

                assert(cacheTileState->status == eTileStatusPending);
                cacheTileState->status = eTileStatusRendered;
            }
        }
    }

    // In persistent mode we have to actually copy the states map from the cache entry to the cache
#ifndef NATRON_CACHE_NEVER_PERSISTENT
    if (stateMapUpdated) {
        updateCachedTilesStateMap();
    } // stateMapUpdated
#endif // NATRON_CACHE_NEVER_PERSISTENT


    // Now add the tiles we just downscaled to our level to the list of tiles to copy
    tilesToCopy.insert(tilesToCopy.end(), perLevelTilesToDownscale[mipMapLevel].begin(), perLevelTilesToDownscale[mipMapLevel].end());
    
    // Finally copy over multiple threads each tile
    FromCachePixelsTransferProcessor processor(effect);
    processor.setValues(localData, tilesToCopy);
    ActionRetCodeEnum stat = processor.launchThreadsBlocking();
    (void)stat;

    return true;

} // fetchAndCopyCachedTiles

void
ImageCacheEntry::fetchCachedTilesAndUpdateStatus(TileStateMap* tileStatus, bool* hasUnRenderedTile, bool *hasPendingResults)
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
            _imp->internalCacheEntry = cacheAccess->getProcessLocalEntry();

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

    for (TileStateVector::const_iterator it = _imp->state.tiles->begin(); it != _imp->state.tiles->end(); ++it) {
        if (it->status == eTileStatusNotRendered) {
            *hasUnRenderedTile = true;
        } else if (it->status == eTileStatusPending) {
            *hasPendingResults = true;
        }
    }
    if (tileStatus) {
        *tileStatus = _imp->state;
    }
} // fetchCachedTilesAndUpdateStatus

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
ImageCacheEntry::markCacheTilesAsRendered()
{
    // Make sure to call updateTilesRenderState() first
    assert(_imp->internalCacheEntry);


#ifdef NATRON_CACHE_NEVER_PERSISTENT
    // In non-persistent mode, lock the cache entry since it's shared across threads.
    // In persistent mode the entry is copied in fromMemorySegment
    boost::unique_lock<boost::shared_mutex> writeLock(_imp->internalCacheEntry->perMipMapTilesStateMutex);
#endif
    // We should have gotten the state map from the cache in fetchCachedTilesAndUpdateStatus()
    assert(!_imp->internalCacheEntry->perMipMapTilesState.empty());

    // This is the tiles state at the mipmap level of interest in the cache
    TileStateMap cacheStateMap(_imp->state.tileSizeX, _imp->state.tileSizeY, _imp->state.bounds, &_imp->internalCacheEntry->perMipMapTilesState[_imp->mipMapLevel]);
    assert(!cacheStateMap.tiles->empty());

    // Read the cache map and update our local map
    CachePtr cache = _imp->internalCacheEntry->getCache();
    bool hasModifiedTileMap = false;

    std::vector<boost::shared_ptr<TileData> > tilesToCopy;
    for (int ty = _imp->roiRoundedToTileSize.y1; ty < _imp->roiRoundedToTileSize.y2; ty += _imp->state.tileSizeY) {
        for (int tx = _imp->roiRoundedToTileSize.x1; tx < _imp->roiRoundedToTileSize.x2; tx += _imp->state.tileSizeX) {

            assert(tx % _imp->state.tileSizeX == 0 && ty % _imp->state.tileSizeY == 0);

            TileState* localTileState = _imp->state.getTileAt(tx, ty);

            if (localTileState->status == eTileStatusNotRendered) {

                localTileState->status = eTileStatusRendered;

                TileState* cacheTileState = cacheStateMap.getTileAt(tx, ty);


                // We marked the cache tile status to eTileStatusPending previously in
                // readAndUpdateStateMap
                // Mark it as eTileStatusRendered now
                assert(cacheTileState->status == eTileStatusPending);
                cacheTileState->status = eTileStatusRendered;

                hasModifiedTileMap = true;

                for (int c = 0; c < _imp->localData.nComps; ++c) {
                    // Mark this tile in the list of tiles to copy
                    boost::shared_ptr<TileData> copy(new TileData);
                    copy->bounds.x1 = tx;
                    copy->bounds.y1 = ty;
                    copy->bounds.x2 = tx + _imp->state.tileSizeX;
                    copy->bounds.y2 = ty + _imp->state.tileSizeX;
                    copy->channel_i = c;
                    tilesToCopy.push_back(copy);
                }
            }
        }
    }

    if (!hasModifiedTileMap) {
        return;
    }


    std::size_t nMonoChannelBuffers = tilesToCopy.size();

    // Allocated buffers for tiles
    std::vector<std::pair<int, void*> > allocatedTiles;
    void* cacheData;
    bool gotTiles = cache->retrieveAndLockTiles(_imp->internalCacheEntry, 0 /*existingTiles*/, nMonoChannelBuffers, NULL, &allocatedTiles, &cacheData);
    CacheDataLock_RAII cacheDataDeleter(cache, cacheData);
    if (!gotTiles) {
        return;
    }

    // Set the tile pointer to the tiles to copy
    for (std::size_t i = 0; i < tilesToCopy.size(); ++i) {
        tilesToCopy[i]->ptr = allocatedTiles[i].second;

        tilesToCopy[i]->tileCache_i = allocatedTiles[i].first;
        // update the tile indices
        int tx = tilesToCopy[i]->bounds.x1;
        int ty = tilesToCopy[i]->bounds.y1;
        TileState* cacheTileState = cacheStateMap.getTileAt(tx, ty);
        cacheTileState->channelsTileStorageIndex[tilesToCopy[i]->channel_i] = allocatedTiles[i].first;

        TileState* localTileState = _imp->state.getTileAt(tx, ty);
        localTileState->channelsTileStorageIndex[tilesToCopy[i]->channel_i] = allocatedTiles[i].first;
    }


    // Finally copy over multiple threads each tile
    ToCachePixelsTransferProcessor processor(_imp->effect);
    processor.setValues(_imp->localData, tilesToCopy);
    ActionRetCodeEnum stat = processor.launchThreadsBlocking();
    (void)stat;

    // In persistent mode we have to actually copy the cache entry tiles state map to the cache
#ifndef NATRON_CACHE_NEVER_PERSISTENT
    _imp->updateCachedTilesStateMap();
#endif // NATRON_CACHE_NEVER_PERSISTENT
} // markCacheTilesAsRendered

#ifndef NATRON_CACHE_NEVER_PERSISTENT

static void toMemorySegmentInternal(const PerMipMapTileStateVector& localMipMapStates, PerMipMapTileStateVector* cachedMipMapStates, ExternalSegmentType* segment)
{
    void_allocator alloc(segment->get_segment_manager());
    cachedMipMapStates->clear();
    for (std::size_t i = 0; i < localMipMapStates.size(); ++i) {
        TileStateVector mipMapStateMap(alloc);
        mipMapStateMap = localMipMapStates[i];
        cachedMipMapStates->push_back(mipMapStateMap);
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
        PerMipMapTileStateVector* mipMapStates = (PerMipMapTileStateVector*)segment->get_address_from_handle(*start);
        assert(mipMapStates->get_allocator().get_segment_manager() == segment->get_segment_manager());
        ++start;

        if (nextFromMemorySegmentCallIsToMemorySegment) {
            // We have written the state map from within markCacheTilesAsRendered(), update it if possible
            if (!isLockedForWriting) {
                // We must update the state map but cannot do so under a read lock
                return CacheEntryBase::eFromMemorySegmentRetCodeNeedWriteLock;
            }

            toMemorySegmentInternal(perMipMapTilesState, mipMapStates, segment);
        } else {

            // Copy the tiles states locally
            perMipMapTilesState = *mipMapStates;

            // Read the state map: it might modify it by marking tiles pending
            ImageCacheEntryPrivate::UpdateStateMapRetCodeEnum stat = _imp->readAndUpdateStateMap(isLockedForWriting);
            switch (stat) {
                case ImageCacheEntryPrivate::eUpdateStateMapRetCodeUpToDate:
                    // We did not write anything on the state map, we are good
                    break;
                case ImageCacheEntryPrivate::eUpdateStateMapRetCodeMustWriteToCache: {
                    // We have written the state map, we must update it
                    assert(isLockedForWriting);
                    toMemorySegmentInternal(perMipMapTilesState, mipMapStates, segment);

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

        PerMipMapTileStateVector *mipMapStates = segment->construct<PerMipMapTileStateVector>(boost::interprocess::anonymous_instance)(alloc);
        if (!mipMapStates) {
            throw std::bad_alloc();
        }
        toMemorySegmentInternal(perMipMapTilesState, mipMapStates, segment);
        objectPointers->push_back(segment->get_handle_from_address(mipMapStates));
    }
    CacheEntryBase::toMemorySegment(segment, objectPointers);
} // toMemorySegment

#endif // #ifndef NATRON_CACHE_NEVER_PERSISTENT

NATRON_NAMESPACE_EXIT;
