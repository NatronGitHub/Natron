/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * Copyright (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
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
#include <boost/thread/mutex.hpp> // local  mutex
#include <boost/thread/locks.hpp>

#include "Engine/AppManager.h"
#include "Engine/Cache.h"
#include "Engine/CacheEntryBase.h"
#include "Engine/Hash64.h"
#include "Engine/Node.h"
#include "Engine/ImageCacheKey.h"
#include "Engine/ImageStorage.h"
#include "Engine/ImagePrivate.h"
#include "Engine/ImageCacheEntryProcessing.h"
#include "Engine/ImageTilesState.h"
#include "Engine/MultiThread.h"
#include "Engine/ThreadPool.h"
#include "Engine/TreeRenderQueueManager.h"
#include "Engine/Timer.h"

// Define to log tiles status in the console
//#define TRACE_TILES_STATUS
//#define TRACE_TILES_ONLY_PRINT_FIRST_TILE
//#define TRACE_RENDERED_TILES
//#define TRACE_TILES_STATUS_SHORT

#if defined(TRACE_TILES_STATUS) || defined(TRACE_TILES_STATUS_SHORT)
#include <QTextStream>
#endif

NATRON_NAMESPACE_ENTER

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

struct TileInternalIndexU64Converter
{
    TileInternalIndexU64Converter()
    : raw(0)
    {
        assert(sizeof(TileInternalIndex) <= sizeof(U64));
    };

    union
    {
        U64 raw;
        TileInternalIndex index;
    };
};

typedef std::set<TileCoord, TileCoordCompare> TilesSet;

struct TileCacheIndex
{
    // Indices of the tiles in the cache for each channel
    TileInternalIndex perChannelTileIndices[4];

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
#if 0
        TileInternalIndex i;
        i.index = (U64)-1;
        perChannelTileIndices[0] = perChannelTileIndices[1] = perChannelTileIndices[2] = perChannelTileIndices[3] = i;
#endif
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

    int tileSizeX, tileSizeY;


public:

    ImageCacheEntryInternalBase()
    : CacheEntryBase(appPTR->getTileCache())
    , perMipMapTilesState()
    , tileSizeX(0)
    , tileSizeY(0)
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

    // This is for each mipmap level, a list of tiles that are allowed to be modified in the implementation of toMemorySegmentInternal
    // So we do not write over tiles that we did not own
    std::vector<TilesSet> tilesToUpdate;

    // If true, only tiles in tilesToUpdate will be updated, otherwise everything local will be updated to the cache
    bool updateAllTilesRegardless;

private:
    ImageCacheEntryInternal(ImageCacheEntryPrivate* _imp)
    : ImageCacheEntryInternalBase()
    , _imp(_imp)
    , nextFromMemorySegmentCallIsToMemorySegment(false)
    , tilesToUpdate()
    , updateAllTilesRegardless(false)
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
    TileInternalIndex tileCache_i;
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

    // The RoD in pixel for each mipmap level up to mipMaplevel
    std::vector<RectI> perMipMapPixelRod;

    // Whether or not we are rendering a low quality version of the image
    bool isDraftModeEnabled;

    // The image buffers, used when the localBuffers is not yet set
    ImageStorageBasePtr imageBuffers[4];

    int nComps;

    // The bitdepth
    ImageBitDepthEnum bitdepth;

    ImageBufferLayoutEnum format;

    // Pointer to the effect for fast abort when copying tiles from/to the cache
    EffectInstanceWPtr effect;

    // The mipmap level we are interested in
    unsigned int mipMapLevel;

    // The key corresponding to this ImageCacheEntry
    ImageCacheKeyPtr key;

    // State indicating the render status of each tiles at the mipMapLevel
    // The state is shared accross all channels because OpenFX doesn't allows yet to render only some channels
    // This state is local to this ImageCacheEntry object: any tile marked eTileStatusPending may be pending because
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
    std::vector<TileCacheIndex> tilesToFetch, tilesToDownscale;

    // If set to eCacheAccessModeWriteOnly the entry will be removed from the cache before reading it so we get a clean image
    // If set to eCacheAccessModeNone, a local tiles state will be created without looking up the cache.
    // If set to eCacheAccessModeReadWrite, the entry will be read from the cache and written to if needed
    CacheAccessModeEnum cachePolicy;

    // True if we should not mark any tile pending within the next call to readAndUpdateStateMap()
    bool updateStateMapReadOnly;

    // Pointer to the image holding this ImageCacheEntry
    ImageWPtr image;

#if defined(TRACE_TILES_STATUS) || defined(TRACE_TILES_STATUS_SHORT)
    QString debugId;
#endif

    ImageCacheEntryPrivate(ImageCacheEntry* publicInterface,
                           const ImagePtr& image,
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
                           CacheAccessModeEnum cachePolicy)
    : _publicInterface(publicInterface)
    , roi(roi)
    , perMipMapPixelRod(mipMapPixelRods)
    , isDraftModeEnabled(isDraft)
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
    , tilesToDownscale()
    , cachePolicy(cachePolicy)
    , updateStateMapReadOnly(false)
    , image(image)
    {
        assert(perMipMapPixelRod.size() >= mipMapLevel + 1);
        for (int i = 0; i < 4; ++i) {
            imageBuffers[i] = storage[i];
        }

        assert(nComps > 0);
        int tileSizeX, tileSizeY;
        CacheBase::getTileSizePx(depth, &tileSizeX, &tileSizeY);

#ifndef NDEBUG
        assert(perMipMapPixelRod[mipMapLevel].contains(roi));
        // We must only compute rectangles that are a multiple of the tile size
        // otherwise we might end up in a situation where a partial tile could be rendered
        assert(roi.x1 % tileSizeX == 0 || roi.x1 == perMipMapPixelRod[mipMapLevel].x1);
        assert(roi.y1 % tileSizeY == 0 || roi.y1 == perMipMapPixelRod[mipMapLevel].y1);
        assert(roi.x2 % tileSizeX == 0 || roi.x2 == perMipMapPixelRod[mipMapLevel].x2);
        assert(roi.y2 % tileSizeY == 0 || roi.y2 == perMipMapPixelRod[mipMapLevel].y2);
#endif

        localTilesState.init(tileSizeX, tileSizeY, perMipMapPixelRod[mipMapLevel]);

        // If we don't want to use the cache, still create a local object used to sync threads
        if (cachePolicy == eCacheAccessModeNone) {
            internalCacheEntry = ImageCacheEntryInternal<false>::create(key);
            internalCacheEntry->tileSizeX = localTilesState.tileSizeX;
            internalCacheEntry->tileSizeY = localTilesState.tileSizeY;
        }

#if defined(TRACE_TILES_STATUS) || defined(TRACE_TILES_STATUS_SHORT)
        QTextStream ts(&debugId);
        if (effect) {
            ts << " " << effect.get() << " " << effect->getScriptName_mt_safe().c_str();
        }
        if (image) {
            ts << " " << image->getLayer().getPlaneLabel().c_str();
        }
        if (internalCacheEntry) {
            ts << " " << internalCacheEntry->getHashKey();
        }
        if (cachePolicy == eCacheAccessModeNone) {
            ts << " (No caching)";
        }
#endif
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
    UpdateStateMapRetCodeEnum readAndUpdateStateMap(bool hasExclusiveLock, bool allTilesMustBeNotRendered) WARN_UNUSED_RETURN;


    /**
     * @brief Fetch from the cache tiles that are cached. This must be called after a call to readAndUpdateStateMap
     * Note that this function does the pixels transfer
     **/
    ActionRetCodeEnum fetchAndCopyCachedTiles() WARN_UNUSED_RETURN;

    /**
     * @brief Mark pending tiles as non rendered. Returns true if at least one tile state was changed.
     **/
    bool markCacheEntriesAsAbortedInternal();

    /**
     * @brief Only relevant if the cache entry is persistent: update the cache from our local cache entry
     **/
    void updateCachedTilesStateMap(const std::vector<TilesSet>& tilesToUpdate, bool updateAllTilesRegardless);

    enum LookupTileStateRetCodeEnum
    {
        eLookupTileStateRetCodeUpToDate,
        eLookupTileStateRetCodeUpdated,
        eLookupTileStateRetCodeNeedWriteLock
    };

    static LookupTileStateRetCodeEnum lookupTileStateInPyramidRecursive(
#ifdef TRACE_TILES_STATUS
                                                                        const QString& debugId,
#endif
                                                                        bool allTilesMustBeNotRendered,
                                                                        bool hasExclusiveLock,
                                                                        std::vector<TileStateHeader>& perMipMapTilesState,
                                                                        unsigned int mipMapLevel,
                                                                        TileCoord coord,
                                                                        unsigned int originalMipMapLevel,
                                                                        bool isDraftModeEnabled,
                                                                        int nComps,
                                                                        std::vector<TileCacheIndex>& localTilesToFetch,
                                                                        std::vector<TileCacheIndex>& localTilesToDownscale,
                                                                        TileStateHeader& localTilesState,
                                                                        std::vector<TilesSet>& localMarkedTiles,
                                                                        bool* hasPendingTile,
                                                                        TileCacheIndex* tile,
                                                                        TileStatusEnum* status);

    static LookupTileStateRetCodeEnum lookupTileStateInPyramid(
#ifdef TRACE_TILES_STATUS
                                                               const QString& debugId,
#endif
                                                               bool allTilesMustBeNotRendered,
                                                               bool hasExclusiveLock,
                                                               std::vector<TileStateHeader>& perMipMapTilesState,
                                                               TileCoord coord,
                                                               unsigned int originalMipMapLevel,
                                                               bool isDraftModeEnabled,
                                                               int nComps,
                                                               std::vector<TileCacheIndex>& localTilesToFetch,
                                                               std::vector<TileCacheIndex>& localTilesToDownscale,
                                                               TileStateHeader& localTilesState,
                                                               std::vector<TilesSet>& localMarkedTiles,
                                                               bool* hasPendingTile);

    std::vector<boost::shared_ptr<TileData> > buildTaskPyramidRecursive(unsigned int lookupLevel,
                                                                        const TileCacheIndex &tile,
                                                                        const std::vector<void*> &fetchedExistingTiles,
                                                                        const std::vector<std::pair<TileInternalIndex, void*> >& allocatedTiles,
                                                                        int* existingTiles_i,
                                                                        int* allocatedTiles_i,
                                                                        std::vector<boost::shared_ptr<TileData> > *tilesToCopy,
                                                                        std::vector<std::vector<boost::shared_ptr<DownscaleTile> > >  *downscaleTilesPerLevel);

#if defined(TRACE_TILES_STATUS) || defined(TRACE_TILES_STATUS_SHORT)
    void writeDebugStatus(const std::string& function, bool entering);
#endif



};

ImageCacheEntry::ImageCacheEntry(const ImagePtr& image,
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
                                 CacheAccessModeEnum cachePolicy)
: _imp(new ImageCacheEntryPrivate(this, image, mipMapPixelRods, roi, isDraft, mipMapLevel, depth, nComps, storage, format, effect, key, cachePolicy))
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

static void growTilesState(const RectI& newPixelRoD, TileStateHeader* stateToGrow)
{
    if (newPixelRoD == stateToGrow->state->bounds) {
        return;
    }

    TileStateHeader tmpHeader;
    tmpHeader.init(stateToGrow->tileSizeX, stateToGrow->tileSizeY, newPixelRoD);

    // If any tile is marked rendered locally, update the copied state
    for (int ty = tmpHeader.state->boundsRoundedToTileSize.y1; ty < tmpHeader.state->boundsRoundedToTileSize.y2; ty += tmpHeader.tileSizeY) {
        for (int tx = tmpHeader.state->boundsRoundedToTileSize.x1; tx < tmpHeader.state->boundsRoundedToTileSize.x2; tx += tmpHeader.tileSizeX) {

            const TileState* state = stateToGrow->getTileAt(tx, ty);
            if (!state) {
                continue;
            }
            TileState* thisState = tmpHeader.getTileAt(tx, ty);
            assert(thisState);
            // Copy the tile state, unless the bounds of the tile changed
            if (state->bounds == thisState->bounds) {
                *thisState = *state;
            }
        }
    }
    *stateToGrow->state = *tmpHeader.state;
}

void
ImageCacheEntry::updateRenderCloneAndImage(const ImagePtr& image, const EffectInstancePtr& newRenderClone)
{
    _imp->effect = newRenderClone;
    _imp->image = image;
}

void
ImageCacheEntry::ensureRoI(const RectI& roi,
                           const ImageStorageBasePtr storage[4],
                           const std::vector<RectI>& perMipMapLevelRoDPixel)
{

#if defined(TRACE_TILES_STATUS) || defined(TRACE_TILES_STATUS_SHORT)
    _imp->writeDebugStatus("ensureRoI", true);
#endif

    // We assume fetchCachedTilesAndUpdateStatus() was called once at least
    assert(_imp->internalCacheEntry);

    // Protect all local structures against multiple threads using this object.
    boost::unique_lock<boost::mutex> locker(_imp->lock);

    for (int i = 0; i < 4; ++i) {
        _imp->imageBuffers[i] = storage[i];
    }

    // Grow the local tiles state map
    _imp->roi.merge(roi);
    _imp->perMipMapPixelRod = perMipMapLevelRoDPixel;
    assert(perMipMapLevelRoDPixel.size() >= _imp->mipMapLevel + 1);
    assert(perMipMapLevelRoDPixel[_imp->mipMapLevel].contains(_imp->localTilesState.state->bounds));
    growTilesState(perMipMapLevelRoDPixel[_imp->mipMapLevel], &_imp->localTilesState);
    assert(_imp->localTilesState.state->bounds.contains(_imp->roi));


    // In non cached, we can grow the tiles state map because it is local (but maybe shared by multiple threads
    // However in cached mode, we need to ensure that the cache holds the same local map.
    // Since there's no assumption on the state of the local map, we do the following:
    // 1 - We mark all tiles that we previously marked pending as non rendered to ensure we don't push pending tiles to the cache
    // 2 - We mark grow the tiles state
    // 3 - We push the new tiles state to the cache

    bool mustUpdateCache = _imp->cachePolicy != eCacheAccessModeNone && _imp->internalCacheEntry->isPersistent();
    if (mustUpdateCache) {
        mustUpdateCache = _imp->markCacheEntriesAsAbortedInternal();
    }

    // For a cached non persistent entry, we must take the entry lock because the internalCacheEntry may be shared by multiple instances
    ImageCacheEntryInternal<false>* nonPersistentLocalEntry = dynamic_cast<ImageCacheEntryInternal<false>* >(_imp->internalCacheEntry.get());

    {
        boost::scoped_ptr<boost::unique_lock<boost::shared_mutex> > writeLock;
        if (nonPersistentLocalEntry && _imp->cachePolicy != eCacheAccessModeNone) {
            writeLock.reset(new boost::unique_lock<boost::shared_mutex>(nonPersistentLocalEntry->perMipMapTilesStateMutex));
        }
        std::size_t nDims = std::min(perMipMapLevelRoDPixel.size(), _imp->internalCacheEntry->perMipMapTilesState.size());
        for (std::size_t i = 0; i < nDims; ++i) {
            TileStateHeader cachedTilesState(_imp->localTilesState.tileSizeX, _imp->localTilesState.tileSizeY, &_imp->internalCacheEntry->perMipMapTilesState[i]);
            growTilesState(perMipMapLevelRoDPixel[i], &cachedTilesState);
        }
    }

    if (mustUpdateCache) {
        _imp->updateCachedTilesStateMap(_imp->markedTiles, false);
        _imp->markedTiles.clear();
    }
} // ensureRoI

#if defined(TRACE_TILES_STATUS) || defined(TRACE_TILES_STATUS_SHORT)
void
ImageCacheEntryPrivate::writeDebugStatus(const std::string& function, bool entering)
{
    
    {
        QString message;
        QTextStream ts(&message);
        ts << QThread::currentThread() << " " << QThread::currentThread()->objectName() << " " << debugId;
        if (entering) {
            ts << " : Entering ";
        } else {
            ts << " : Leaving ";
        }
        ts << function.c_str() << " at level " << mipMapLevel;
        qDebug() << message;
    }
}
#endif


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
            
            //assert(task.tileCache_i.index != (U64)-1);

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
                /*{
                    QString message = QString::fromUtf8("COPYING bucket %1 tileIndex %2 from file index %3").arg((int)task.tileCache_i.bucketIndex).arg((int)task.tileCache_i.index.tileIndex).arg((int)task.tileCache_i.index.fileIndex);
                    qDebug() << message;
                }*/
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
            // Commented-out: Since cache indices have already been allocated at this point, it would be costly to then release them to the cache, instead
            // we let the downscaling finish (which is fast anyway)
            /*if (_effect && _effect->isRenderAborted()) {
                return eActionStatusAborted;
            }*/

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
    bool invalidated;
    CacheDataLock_RAII(const CacheBasePtr& cache, void* cacheData)
    : data(cacheData)
    , cache(cache)
    , invalidated(false)
    {

    }

    void invalidateTiles() {
        invalidated = true;
    }


    ~CacheDataLock_RAII()
    {
        if (data) {
            cache->unLockTiles(data, invalidated);
        }
    }
};

ImageCacheEntryPrivate::LookupTileStateRetCodeEnum
ImageCacheEntryPrivate::lookupTileStateInPyramid(
#ifdef TRACE_TILES_STATUS
                                                 const QString& debugId,
#endif
                                                 bool allTilesMustBeNotRendered,
                                                 bool hasExclusiveLock,
                                                 std::vector<TileStateHeader>& perMipMapTilesState,
                                                 TileCoord coord,
                                                 unsigned int originalMipMapLevel,
                                                 bool isDraftModeEnabled,
                                                 int nComps,
                                                 std::vector<TileCacheIndex>& localTilesToFetch,
                                                 std::vector<TileCacheIndex>& localTilesToDownscale,
                                                 TileStateHeader& localTilesState,
                                                 std::vector<TilesSet>& localMarkedTiles,
                                                 bool* hasPendingTile)
{
    TileCacheIndex tile;
    TileStatusEnum status;
    return lookupTileStateInPyramidRecursive(
#ifdef TRACE_TILES_STATUS
                                             debugId,
#endif
                                             allTilesMustBeNotRendered,
                                             hasExclusiveLock, perMipMapTilesState, originalMipMapLevel, coord, originalMipMapLevel, isDraftModeEnabled, nComps, localTilesToFetch, localTilesToDownscale, localTilesState, localMarkedTiles, hasPendingTile, &tile, &status);
}

static void removeTileFromMarkedTiles(std::vector<TileStateHeader>& perMipMapTilesState, std::vector<TilesSet>& markedTiles, unsigned int mipMapLevel, const TileCacheIndex& tile)
{
    for (int i = 0; i < 4; ++i) {
        if (tile.upscaleTiles[i]) {
            removeTileFromMarkedTiles(perMipMapTilesState, markedTiles, mipMapLevel - 1, *tile.upscaleTiles[i]);
        }
    }
    TileCoord coord = {tile.tx, tile.ty};
    TilesSet::iterator found = markedTiles[mipMapLevel].find(coord);
    if (found != markedTiles[mipMapLevel].end()) {
        markedTiles[mipMapLevel].erase(found);

        TileState* cacheTileState = perMipMapTilesState[mipMapLevel].getTileAt(coord.tx, coord.ty);
        assert(cacheTileState);
        assert(cacheTileState->status == eTileStatusPending);
        cacheTileState->status = eTileStatusNotRendered;
    }
}

ImageCacheEntryPrivate::LookupTileStateRetCodeEnum
ImageCacheEntryPrivate::lookupTileStateInPyramidRecursive(
#ifdef TRACE_TILES_STATUS
                                                          const QString& debugId,
#endif
                                                          bool allTilesMustBeNotRendered,
                                                          bool hasExclusiveLock,
                                                          std::vector<TileStateHeader>& perMipMapTilesState,
                                                          unsigned int lookupLevel,
                                                          TileCoord coord,
                                                          unsigned int originalMipMapLevel,
                                                          bool isDraftModeEnabled,
                                                          int nComps,
                                                          std::vector<TileCacheIndex>& localTilesToFetch,
                                                          std::vector<TileCacheIndex>& localTilesToDownscale,
                                                          TileStateHeader& localTilesState,
                                                          std::vector<TilesSet>& localMarkedTiles,
                                                          bool* hasPendingTile,
                                                          TileCacheIndex* tile,
                                                          TileStatusEnum* status)
{
    *status = eTileStatusNotRendered;

    if (localMarkedTiles[lookupLevel].find(coord) != localMarkedTiles[lookupLevel].end()) {
        // We may already marked this tile before
        return eLookupTileStateRetCodeUpToDate;
    }

    // Find this tile in the current state
    TileState* cacheTileState = perMipMapTilesState[lookupLevel].getTileAt(coord.tx, coord.ty);
    assert(cacheTileState);
    *status = cacheTileState->status;

    (void)allTilesMustBeNotRendered;
    assert(!allTilesMustBeNotRendered || *status == eTileStatusNotRendered);

    // There's a local tile state only for the requested mipmap level
    TileState* localTileState = (lookupLevel == originalMipMapLevel) ? localTilesState.getTileAt(coord.tx, coord.ty) : 0;
    tile->tx = coord.tx;
    tile->ty = coord.ty;

    // A tile is not considered rendered if it is in low quality but the image is not flagged with low quality
    if (*status == eTileStatusRenderedLowQuality && !isDraftModeEnabled) {
        *status = eTileStatusNotRendered;
    }

    if (*status == eTileStatusPending) {
        // If a tile is pending, check if the compute process is still active, otherwise mark it not rendered
        CacheBasePtr cache = appPTR->getTileCache();
        if (cache->isPersistent() && !cache->isUUIDCurrentlyActive(cacheTileState->uuid)) {
            *status = eTileStatusNotRendered;
        }
    }

    switch (*status) {
        case eTileStatusRenderedHighestQuality:
        case eTileStatusRenderedLowQuality: {
            assert(cacheTileState);
            for (int k = 0; k < nComps; ++k) {
                tile->perChannelTileIndices[k] = cacheTileState->channelsTileStorageIndex[k];
                //assert(tile->perChannelTileIndices[k].index != (U64)-1);
            }

            // If the tile has become rendered at the requested mipmap level, mark it in the tiles to
            // fetch list. This will be fetched later on (when outside of the cache
            // scope) in fetchAndCopyCachedTiles()
            if (localTileState) {
                localTilesToFetch.push_back(*tile);
                // Locally, update the status to rendered
                localTileState->status = *status;
            }

#if defined(TRACE_TILES_STATUS) && defined(TRACE_RENDERED_TILES)
#ifdef TRACE_TILES_ONLY_PRINT_FIRST_TILE
            if (coord.tx == 0 && coord.ty == 0)
#endif
                qDebug() << QThread::currentThread() << debugId << "lookup(): tile " << coord.tx << coord.ty << "is rendered in cache at level" << lookupLevel;
#endif
            assert(!allTilesMustBeNotRendered);
            return eLookupTileStateRetCodeUpToDate;
        }
        case eTileStatusPending: {
            assert(cacheTileState);
            // If the tile is pending in the cache, leave it pending locally
            if (localTileState) {
                localTileState->status = eTileStatusPending;
                *hasPendingTile = true;
            }
            *status = eTileStatusPending;
#ifdef TRACE_TILES_STATUS
#ifdef TRACE_TILES_ONLY_PRINT_FIRST_TILE
            if (coord.tx == 0 && coord.ty == 0)
#endif
                qDebug() << QThread::currentThread() << debugId << "lookup(): tile " << coord.tx << coord.ty << "is pending in cache at level" << lookupLevel;
#endif
            assert(!allTilesMustBeNotRendered);
            return eLookupTileStateRetCodeUpToDate;

        }
        case eTileStatusNotRendered: {


            // If we are in a mipmap level > 0, check in higher scales if the image is not yet available, in which
            // case we just have to downscale by a power of 2 (4 tiles become 1 tile)
            // If the upscaled state is not yet initialized, skip it
            TileStatusEnum downscaledTileStatus = eTileStatusNotRendered;
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
                downscaledTileStatus = eTileStatusRenderedHighestQuality;

                int nInvalid = 0;
                for (int i = 0; i < 4; ++i) {

                    tile->upscaleTiles[i].reset(new TileCacheIndex);
                    // The upscaled tile might not exist when we are on the border, account for it
                    if (upscaledCords[i].tx < perMipMapTilesState[lookupLevel -1].state->boundsRoundedToTileSize.x1 ||
                        upscaledCords[i].tx >= perMipMapTilesState[lookupLevel -1].state->boundsRoundedToTileSize.x2 ||
                        upscaledCords[i].ty < perMipMapTilesState[lookupLevel -1].state->boundsRoundedToTileSize.y1 ||
                        upscaledCords[i].ty >= perMipMapTilesState[lookupLevel -1].state->boundsRoundedToTileSize.y2) {
                        ++nInvalid;
                        continue;
                    }

                    // Recurse on the higher scale tile and get its status
                    TileStatusEnum higherScaleStatus;
                    LookupTileStateRetCodeEnum recurseRet = lookupTileStateInPyramidRecursive(
#ifdef TRACE_TILES_STATUS
                                                                                              debugId,
#endif
                                                                                              allTilesMustBeNotRendered,
                                                                                              hasExclusiveLock, perMipMapTilesState, lookupLevel - 1, upscaledCords[i], originalMipMapLevel, isDraftModeEnabled, nComps, localTilesToFetch, localTilesToDownscale, localTilesState, localMarkedTiles, hasPendingTile, tile->upscaleTiles[i].get(), &higherScaleStatus);
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
                            downscaledTileStatus = eTileStatusNotRendered;
                            break;
                        case eTileStatusRenderedHighestQuality:
                            // Higher scale tile is already rendered
                            break;
                        case eTileStatusRenderedLowQuality:
                            // Higher scale tile is already rendered but with lower quality
                            downscaledTileStatus = eTileStatusRenderedLowQuality;
                            break;
                        case eTileStatusPending:
                            downscaledTileStatus = eTileStatusPending;
                            break;
                    }
                    if (downscaledTileStatus == eTileStatusNotRendered) {
                        break;
                    }
                } // for each upscaled mipmap

                // We may be in a non valid tile, e.g:
                //
                // Tile Size: 64 * 64
                // Pixel RoD at level 2: -97, -32, 386, 250
                // Pixel RoD at level 2 rounded to tile size: -128, -64, 448, 256

                // Pixel RoD at level 1: -193, 44, 770, 499
                // Pixel RoD at level 1 rounded to tile size: -256, -64, 832, 512
                //
                // Pixel RoD at level 0: -394, -86, 1538, 996
                // Pixel RoD at level 0 rounded to tile size: -384, -128, 1600, 1024
                //
                // If we wanted to lookup at level 2, the tile with its bottom left corner being (-128, -64)
                // We would need to lookup the following tiles at level 1: (-256, -128) (invalid), (-192, -128) (invalid), (-256, -64)(valid), (-192, -64)(valid)
                // To in-turn lookup the valid tile at (-256, -64),
                // We would need to lookup the following tiles at level 0: (-512, -128) (invalid), (-448, -128) (invalid), (-512, -64)(invalid), (-448, -64)(invalid): None of them are valid
                if (nInvalid == 4) {
                    downscaledTileStatus = eTileStatusNotRendered;
                }
                assert(downscaledTileStatus == eTileStatusNotRendered || (nInvalid == 0 || nInvalid == 2 || nInvalid == 3));
            } // lookupLevel > 0

            *status = downscaledTileStatus;
            switch (downscaledTileStatus) {
                case eTileStatusPending:
                    // One or more of the upscaled tile(s) in the pyramid is pending, but all others are rendered. Wait for them to be computed

                    // Remove from pending state the higher scale tiles that could have been marked pending in the recursive call to lookupTileStateInPyramidRecursive
                    for (int i = 0; i < 4; ++i) {
                        if (tile->upscaleTiles[i]) {
                            removeTileFromMarkedTiles(perMipMapTilesState, localMarkedTiles, lookupLevel - 1, *tile->upscaleTiles[i]);
                        }
                    }
                    if (localTileState) {
                        localTileState->status = eTileStatusPending;
                        *hasPendingTile = true;
                    }
#ifdef TRACE_TILES_STATUS
#ifdef TRACE_TILES_ONLY_PRINT_FIRST_TILE
                    if (coord.tx == 0 && coord.ty == 0)
#endif
                    qDebug() << QThread::currentThread() << debugId << "lookup(): tile " << coord.tx << coord.ty << "is pending in cache at level" << lookupLevel << "because another tile is pending in higher scale";
#endif
                    assert(!allTilesMustBeNotRendered);
                    break;
                case eTileStatusRenderedHighestQuality:
                case eTileStatusRenderedLowQuality: {

                    // If we reach here, it is because upscaled tiles are cached
                    assert(tile->upscaleTiles[0]);

                    // All 4 upscaled tiles are rendered, just fetch all upscaled tiles and downscale afterwards
                    // If this tile is not yet rendered at this level but we found a higher scale mipmap,
                    // we must write this tile as pending in the cache state map so that another thread does not attempt to downscale too
                    // We need an exclusive lock to do so
                    if (!hasExclusiveLock) {
                        return eLookupTileStateRetCodeNeedWriteLock;
                    }

                    // Mark the tile with the UUID of the cache so we know by which process this tile is computed.
                    boost::uuids::uuid sessionUUID = appPTR->getTileCache()->getCurrentProcessUUID();

                    if (localTileState) {
                        // Mark it not rendered locally, this will be switched to rendered once the downscale operation is performed
                        localTileState->status = eTileStatusNotRendered;

                        localTileState->uuid = sessionUUID;

                        // Only append in the localTilesToDownscale list at the originally requested mipmap level:
                        // the fetchTileIndicesInPyramid() function will recursively fetch tiles at higher scale
                        localTilesToDownscale.push_back(*tile);

                    }



                    cacheTileState->status = eTileStatusPending;
                    cacheTileState->uuid = sessionUUID;

                    // Mark this tile so it is found quickly in markCacheTilesAsAborted()
                    assert(coord.tx == tile->tx && coord.ty == tile->ty);
                    localMarkedTiles[lookupLevel].insert(coord);
#ifdef TRACE_TILES_STATUS
#ifdef TRACE_TILES_ONLY_PRINT_FIRST_TILE
                    if (coord.tx == 0 && coord.ty == 0)
#endif
                    qDebug() << QThread::currentThread() << debugId << "lookup(): marking tile " << coord.tx << coord.ty << "pending (downscaled from 4 higher scale tiles) at level" << lookupLevel;
#endif
                    retCode = eLookupTileStateRetCodeUpdated;
                    assert(!allTilesMustBeNotRendered);
                }   break;
                case eTileStatusNotRendered:

                    // We must render the tile. If we are on the originally requested
                    // mipmap level, mark the tile pending. Otherwise if this is a higher scale tile and it is not rendered,
                    // there's no point to mark it: we can render directly the downscale tile.

                    // Remove from pending state the higher scale tiles that could have been marked pending in the recursive call to lookupTileStateInPyramidRecursive
                    for (int i = 0; i < 4; ++i) {
                        if (tile->upscaleTiles[i]) {
                            removeTileFromMarkedTiles(perMipMapTilesState, localMarkedTiles, lookupLevel - 1, *tile->upscaleTiles[i]);
                        }
                    }

                    if (lookupLevel == originalMipMapLevel) {

                        // We are going to modify the state, we need exclusive rights
                        if (!hasExclusiveLock) {
                            return eLookupTileStateRetCodeNeedWriteLock;
                        }

                        // Mark it not rendered locally
                        assert(localTileState);
                        localTileState->status = eTileStatusNotRendered;

                        // Mark it pending in the cache
                        cacheTileState->status = eTileStatusPending;

                        boost::uuids::uuid sessionUUID = appPTR->getTileCache()->getCurrentProcessUUID();
                        cacheTileState->uuid = sessionUUID;
                        localTileState->uuid = sessionUUID;

                        // Mark this tile so it is found quickly in markCacheTilesAsAborted()
#ifdef TRACE_TILES_STATUS
#ifdef TRACE_TILES_ONLY_PRINT_FIRST_TILE
                        if (coord.tx == 0 && coord.ty == 0)
#endif
                        qDebug() << QThread::currentThread() << debugId << "lookup(): marking tile " << coord.tx << coord.ty << "pending at level" << lookupLevel;
#endif

                        localMarkedTiles[lookupLevel].insert(coord);

                        retCode = eLookupTileStateRetCodeUpdated;
                    }
                    break;
            }

            return retCode;
        } // eTileStatusNotRendered
    } // switch(foundTile->status)
    throw std::runtime_error("ImageCacheEntryPrivate::lookupTileStateInPyramidRecursive: unknown tile status");
} // lookupTileStateInPyramidRecursive

ImageCacheEntryPrivate::UpdateStateMapRetCodeEnum
ImageCacheEntryPrivate::readAndUpdateStateMap(bool hasExclusiveLock, bool allTilesMustBeNotRendered)
{

    if (hasExclusiveLock && updateStateMapReadOnly) {
        hasExclusiveLock = false;
    }

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
    for (std::size_t i = 0; i < perMipMapCacheTilesState.size(); ++i) {
        //RectI levelBounds = mipmap0Bounds.downscalePowerOfTwo(i);
        RectI boundsRounded = perMipMapPixelRod[i];
        boundsRounded.roundToTileSize(localTilesState.tileSizeX, localTilesState.tileSizeY);

        // If the cached entry does not contain the exact amount of tiles, fail: a thread may have crashed while creating the map.
        std::size_t expectedNumTiles = (boundsRounded.width() / localTilesState.tileSizeX) * (boundsRounded.height() / localTilesState.tileSizeY);
        if (!internalCacheEntry->perMipMapTilesState[i].tiles.empty() && (internalCacheEntry->perMipMapTilesState[i].tiles.size() != expectedNumTiles)) {
            return eUpdateStateMapRetCodeFailed;
        }

        // Wrap the cache entry perMipMapTilesState with a TileStateHeader
        perMipMapCacheTilesState[i] = TileStateHeader(localTilesState.tileSizeX, localTilesState.tileSizeY, &internalCacheEntry->perMipMapTilesState[i]);

        if (internalCacheEntry->perMipMapTilesState[i].tiles.empty()) {

            if (!hasExclusiveLock) {
                return ImageCacheEntryPrivate::eUpdateStateMapRetCodeNeedWriteLock;
            }

            // The state for this mipmap level is not yet initialized in the cache entry
            /*if (i == mipMapLevel) {
                *perMipMapCacheTilesState[i].state = *localTilesState.state;
            } else {*/
                // If the tiles state at the mipmap level is empty, default initialize it
                TileStateHeader tmp;
                tmp.init(localTilesState.tileSizeX, localTilesState.tileSizeY, perMipMapPixelRod[i]);
                *perMipMapCacheTilesState[i].state = *tmp.state;
            //}
#if defined(TRACE_TILES_STATUS)
            qDebug() << QThread::currentThread() << debugId << "init tiles at level " << i;
#endif
            assert(!perMipMapCacheTilesState[i].state->tiles.empty());
            stateMapModified = true;
        }
        
    }


    if (markedTiles.size() != mipMapLevel + 1) {
        markedTiles.resize(mipMapLevel + 1);
    }

    // Work on a temporary copy of the following members if we happen to return early because
    // lookupTileStateInPyramid returned eUpdateStateMapRetCodeNeedWriteLock
    std::vector<TilesSet> tmpMarkedTiles = markedTiles;
    bool tmpHasPendingTiles = false;
    std::vector<TileCacheIndex> tmpTilesToFetch, tmpTilesToDownscale;
    TileStateHeader tmpLocalState;
    {
        tmpLocalState.ownsState = true;
        tmpLocalState.state = new TilesState;
        *tmpLocalState.state = *localTilesState.state;
        tmpLocalState.tileSizeX = localTilesState.tileSizeX;
        tmpLocalState.tileSizeY = localTilesState.tileSizeY;
    }

    // For each tile in the RoI (rounded to the tile size):
    // Check the tile status, only copy from the cache if rendered
    RectI roiRounded = roi;
    roiRounded.roundToTileSize(localTilesState.tileSizeX, localTilesState.tileSizeY);
    for (int ty = roiRounded.y1; ty < roiRounded.y2; ty += localTilesState.tileSizeY) {
        for (int tx = roiRounded.x1; tx < roiRounded.x2; tx += localTilesState.tileSizeX) {

            assert(tx % localTilesState.tileSizeX == 0 && ty % localTilesState.tileSizeY == 0);

            TileState* localTileState = localTilesState.getTileAt(tx, ty);
            assert(localTileState);

            // If the tile in the old status is already rendered, do not update
            if (localTileState->status == eTileStatusRenderedHighestQuality ||
                (localTileState->status == eTileStatusRenderedLowQuality && isDraftModeEnabled)) {
                continue;
            }

            TileCoord coord = {tx, ty};

            // Traverse the mipmaps pyramid from lower scale to higher scale to find a rendered tile and then downscale if necessary
            LookupTileStateRetCodeEnum stat = lookupTileStateInPyramid(
#ifdef TRACE_TILES_STATUS
                                                                       debugId,
#endif
                                                                       allTilesMustBeNotRendered,
                                                                       hasExclusiveLock, perMipMapCacheTilesState, coord, mipMapLevel, isDraftModeEnabled, nComps, tmpTilesToFetch, tmpTilesToDownscale, tmpLocalState, tmpMarkedTiles, &tmpHasPendingTiles);
            switch (stat) {
                case eLookupTileStateRetCodeNeedWriteLock: {
                    return ImageCacheEntryPrivate::eUpdateStateMapRetCodeNeedWriteLock;
                }
                case eLookupTileStateRetCodeUpdated:
                    stateMapModified = true;
                    break;
                case eLookupTileStateRetCodeUpToDate:
                    break;
            }
        }
    }

    // Update local fields
    tilesToFetch = tmpTilesToFetch;
    tilesToDownscale = tmpTilesToDownscale;
    hasPendingTiles = tmpHasPendingTiles;
    markedTiles = tmpMarkedTiles;
    *localTilesState.state = *tmpLocalState.state;

    // If persistent, indicate tiles to update
    ImageCacheEntryInternal<true>* isPersistent = dynamic_cast<ImageCacheEntryInternal<true>*>(internalCacheEntry.get());
    if (isPersistent) {
        isPersistent->tilesToUpdate = markedTiles;
        isPersistent->updateAllTilesRegardless = false;
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
static void fetchTileIndicesInPyramid(U64 entryHash,
                                      unsigned int lookupLevel,
                                      int tileSizeX,
                                      int tileSizeY,
                                      const TileCacheIndex& tile,
                                      int nComps,
                                      std::vector<TileInternalIndex> *tileIndicesToFetch,
#ifdef NATRON_CACHE_TILES_MEMORY_ALLOCATOR_CENTRALIZED
                                      std::size_t* tilesAllocNeeded
#else
                                      std::vector<TileHash>* tilesAllocNeeded
#endif
                                      )
{
    if (tile.upscaleTiles[0]) {
        // We must downscale the upscaled tiles. Allocate 1 tile for the resulting tile
#ifdef NATRON_CACHE_TILES_MEMORY_ALLOCATOR_CENTRALIZED
        *tilesAllocNeeded += nComps;
#else
        for (int c = 0; c < nComps; ++c) {
            TileHash tileBucketHash = CacheBase::makeTileCacheIndex(tile.tx, tile.ty, lookupLevel, c, entryHash);
            tilesAllocNeeded->push_back(tileBucketHash);
        }
#endif

        // For each upscaled tile
        for (int i = 0; i < 4; ++i) {
            assert(tile.upscaleTiles[i]);
            // Check that the upscaled tile exists and recurse: on the edges and corner of the images a tile may not necessarily have 4 upscaled tiles
            if (tile.upscaleTiles[i]->tx != -1) {
                fetchTileIndicesInPyramid(entryHash, lookupLevel - 1, tileSizeX, tileSizeY, *tile.upscaleTiles[i], nComps, tileIndicesToFetch, tilesAllocNeeded);
            }
        }
    } else {
        for (int c = 0; c < nComps; ++c) {
            //assert(tile.perChannelTileIndices[c].index != (U64)-1);
            tileIndicesToFetch->push_back(tile.perChannelTileIndices[c]);
        }
    }
} // fetchTileIndicesInPyramid

/**
 * @brief Build the tasks pyramid for a given tile. This function is analogous to the fetchTileIndicesInPyramid function:
 * it expects the tile indices in the same order as they were given in the other function.
 **/
std::vector<boost::shared_ptr<TileData> >
ImageCacheEntryPrivate::buildTaskPyramidRecursive(unsigned int lookupLevel,
                                                  const TileCacheIndex &tile,
                                                  const std::vector<void*> &fetchedExistingTiles,
                                                  const std::vector<std::pair<TileInternalIndex, void*> >& allocatedTiles,
                                                  int* existingTiles_i,
                                                  int* allocatedTiles_i,
                                                  std::vector<boost::shared_ptr<TileData> > *tilesToCopy,
                                                  std::vector<std::vector<boost::shared_ptr<DownscaleTile> > >  *downscaleTilesPerLevel)
{

    std::vector<boost::shared_ptr<TileData> > outputTasks(nComps);

    RectI thisLevelRoD = internalCacheEntry->perMipMapTilesState[lookupLevel].bounds;

    // The tile bounds are clipped to the pixel RoD
    RectI tileBounds;
    tileBounds.x1 = std::max(tile.tx, thisLevelRoD.x1);
    tileBounds.y1 = std::max(tile.ty, thisLevelRoD.y1);
    tileBounds.x2 = std::min(tile.tx + localTilesState.tileSizeX, thisLevelRoD.x2);
    tileBounds.y2 = std::min(tile.ty + localTilesState.tileSizeY, thisLevelRoD.y2);
    assert(!tileBounds.isNull());
    assert(downscaleTilesPerLevel->size() > lookupLevel);

    if (tile.upscaleTiles[0]) {

        std::vector<boost::shared_ptr<DownscaleTile> > thisLevelTask(nComps);

        // Create tasks at this level
        for (int c = 0; c < nComps; ++c) {
            thisLevelTask[c].reset(new DownscaleTile);

            thisLevelTask[c]->bounds = tileBounds;
            assert(*allocatedTiles_i >= 0 && *allocatedTiles_i < (int)allocatedTiles.size());
            thisLevelTask[c]->ptr = allocatedTiles[*allocatedTiles_i].second;
            thisLevelTask[c]->tileCache_i = allocatedTiles[*allocatedTiles_i].first;
            thisLevelTask[c]->channel_i = c;
            ++(*allocatedTiles_i);
            outputTasks[c] = thisLevelTask[c];
            (*downscaleTilesPerLevel)[lookupLevel].push_back(thisLevelTask[c]);
        }

        // Recurse upstream.
#ifdef DEBUG
        int nInvalidTiles = 0;
#endif
        for (int i = 0; i < 4; ++i) {
            assert(tile.upscaleTiles[i]);

            // Check if the upscale tile exists, if so recurse
            if (tile.upscaleTiles[i]->tx != -1) {
                // For each upscaled tiles, it returns us a vector of exactly nComps tasks.
                std::vector<boost::shared_ptr<TileData> > upscaledTileTasks = buildTaskPyramidRecursive(lookupLevel - 1, *tile.upscaleTiles[i],  fetchedExistingTiles, allocatedTiles, existingTiles_i, allocatedTiles_i, tilesToCopy, downscaleTilesPerLevel);
                assert((int)upscaledTileTasks.size() == nComps);
                for (int c = 0; c < nComps; ++c) {
                    thisLevelTask[c]->srcTiles[i] = upscaledTileTasks[c];
                }

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
        
    } else { // !tile.upscaleTiles[0]
        for (int c = 0; c < nComps; ++c) {
            //assert(tile.perChannelTileIndices[c].index != (U64)-1);


            outputTasks[c].reset(new TileData);
            assert(*existingTiles_i >= 0 && *existingTiles_i < (int)fetchedExistingTiles.size());
            outputTasks[c]->ptr = fetchedExistingTiles[*existingTiles_i];
            ++(*existingTiles_i);
            //assert(tile.perChannelTileIndices[c].index != (U64)-1);
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

    U64 entryHash = internalCacheEntry->getHashKey();

    // Get a vector of tile indices to fetch from the cache directly
    std::vector<TileInternalIndex> tileIndicesToFetch;


    // Number of tiles to allocate to downscale
#ifdef NATRON_CACHE_TILES_MEMORY_ALLOCATOR_CENTRALIZED
    std::size_t tilesAllocNeeded = 0;
#else
    std::vector<TileHash> tilesAllocNeeded;
#endif
    for (std::size_t i = 0; i < tilesToFetch.size(); ++i) {
        // Fetch all tiles that should be copied from the cache. We don't need to allocate any tile in the cache, so pass a NULL pointer for the tilesAllocNeeded
        fetchTileIndicesInPyramid(entryHash, mipMapLevel, localTilesState.tileSizeX, localTilesState.tileSizeY, tilesToFetch[i], nComps, &tileIndicesToFetch, 0);
    }
    for (std::size_t i = 0; i < tilesToDownscale.size(); ++i) {
        // Fetch all tiles to downscale: it will recursively fetch upscaled tiles and mark new tiles to allocate in output
        fetchTileIndicesInPyramid(entryHash, mipMapLevel, localTilesState.tileSizeX, localTilesState.tileSizeY, tilesToDownscale[i], nComps, &tileIndicesToFetch, &tilesAllocNeeded);
    }

    if (tileIndicesToFetch.empty() &&
#ifdef NATRON_CACHE_TILES_MEMORY_ALLOCATOR_CENTRALIZED
        tilesAllocNeeded > 0
#else
        tilesAllocNeeded.empty()
#endif
        ) {
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
    std::vector<std::pair<TileInternalIndex, void*> > allocatedTiles;
    void* cacheData;
    bool gotTiles = tileCache->retrieveAndLockTiles(internalCacheEntry, &tileIndicesToFetch,
#ifdef NATRON_CACHE_TILES_MEMORY_ALLOCATOR_CENTRALIZED
                                                    tilesAllocNeeded,
#else
                                                    &tilesAllocNeeded,
#endif
                                                    &fetchedExistingTiles, &allocatedTiles, &cacheData);
    boost::scoped_ptr<CacheDataLock_RAII> cacheDataDeleter(new CacheDataLock_RAII(tileCache, cacheData));
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
        // The buildTaskPyramidRecursive call for tilesToFetch should not have allocated any tile
        assert(allocatedTiles_i == 0);
        for (std::size_t i = 0; i < tilesToDownscale.size(); ++i) {
            buildTaskPyramidRecursive(mipMapLevel, tilesToDownscale[i], fetchedExistingTiles, allocatedTiles, &existingTiles_i, &allocatedTiles_i, &tilesToCopy, &perLevelTilesToDownscale);
        }
    }

    EffectInstancePtr renderClone = effect.lock();

    // If we downscaled some tiles, we updated the tiles status map
    bool stateMapUpdated = false;

    // Downscale in parallel each mipmap level tiles and then copy the last level tiles
    std::vector<TilesSet> tilesToUpdate(perLevelTilesToDownscale.size());
    for (std::size_t i = 0; i < perLevelTilesToDownscale.size(); ++i) {

        if (perLevelTilesToDownscale[i].empty()) {
            continue;
        }

        TileStateHeader cacheStateMap = TileStateHeader(localTilesState.tileSizeX, localTilesState.tileSizeY, &internalCacheEntry->perMipMapTilesState[i]);
        assert(!cacheStateMap.state->tiles.empty());

        // Downscale all tiles for the same mipmap level concurrently
        boost::scoped_ptr<DownscaleMipMapProcessorBase> processor;
        switch (bitdepth) {
            case eImageBitDepthByte:
                processor.reset(new DownscaleMipMapProcessor<unsigned char>(renderClone));
                break;
            case eImageBitDepthShort:
                processor.reset(new DownscaleMipMapProcessor<unsigned short>(renderClone));
                break;
            case eImageBitDepthFloat:
                processor.reset(new DownscaleMipMapProcessor<float>(renderClone));
                break;
            default:
                assert(false);
                break;
        }
        processor->setValues(localTilesState.tileSizeX, localTilesState.tileSizeY, perLevelTilesToDownscale[i]);
        ActionRetCodeEnum downscaleStatus = processor->launchThreadsBlocking();
        (void)downscaleStatus;
        assert(downscaleStatus == eActionStatusOK);

        stateMapUpdated = true;

        // We downscaled hence we must update tiles status from eTileStatusNotRendered to eTileStatusRendered
        // Only do so for the first channel since they all share the same state

        for (std::size_t j = 0; j  < perLevelTilesToDownscale[i].size(); ++j) {

            const RectI& tileBounds = perLevelTilesToDownscale[i][j]->bounds;

            // Get the bottom left coordinates of the tile
            int tx = (int)std::floor((double)tileBounds.x1 / localTilesState.tileSizeX) * localTilesState.tileSizeX;
            int ty = (int)std::floor((double)tileBounds.y1 / localTilesState.tileSizeY) * localTilesState.tileSizeY;


            TileState* cacheTileState = cacheStateMap.getTileAt(tx, ty);
            //assert(perLevelTilesToDownscale[i][j]->tileCache_i.index != (U64)-1);
            cacheTileState->channelsTileStorageIndex[perLevelTilesToDownscale[i][j]->channel_i] = perLevelTilesToDownscale[i][j]->tileCache_i;

#if 0
            // Check that upon the last channels, all tile indices are correct
            if (perLevelTilesToDownscale[i][j]->channel_i == nComps -1) {
                for (int c = 0; c < nComps; ++c) {
                    assert(cacheTileState->channelsTileStorageIndex[c].index != (U64)-1);
                }
            }
#endif
            
            // Update the tile state only for the first channel
            if (perLevelTilesToDownscale[i][j]->channel_i != 0) {
                continue;
            }


            assert(i != mipMapLevel || cacheTileState->status == eTileStatusPending);

            cacheTileState->status = isDraftModeEnabled ? eTileStatusRenderedLowQuality : eTileStatusRenderedHighestQuality;

            // Remove this tile from the marked tiles
            assert(markedTiles.size() == mipMapLevel + 1);
            TileCoord coord = {tx, ty};
            TilesSet::iterator foundMarked = markedTiles[i].find(coord);
            assert(foundMarked != markedTiles[i].end());
            tilesToUpdate[i].insert(*foundMarked);
            markedTiles[i].erase(foundMarked);
#ifdef TRACE_TILES_STATUS
#ifdef TRACE_TILES_ONLY_PRINT_FIRST_TILE
            if (coord.tx == 0 && coord.ty == 0)
#endif
            qDebug() << QThread::currentThread() << debugId << "marking " << tx << ty << "rendered with downscale at level" << i;
#endif

            // Update the state locally if we are on the appropriate mip map level
            if (i == mipMapLevel) {
                TileState* localTileState = localTilesState.getTileAt(tx, ty);
                //assert(perLevelTilesToDownscale[i][j]->tileCache_i.index != (U64)-1);
                localTileState->channelsTileStorageIndex[perLevelTilesToDownscale[i][j]->channel_i] = perLevelTilesToDownscale[i][j]->tileCache_i;

                assert(localTileState->status == eTileStatusNotRendered);
                localTileState->status = cacheTileState->status;
            }

        } // for each tile


        // All tiles that were marked pending should now be marked rendered, unless we are at the requested mipmap level
        assert(i == mipMapLevel || markedTiles[i].empty());


    } // for each mip map level


    // Now add the tiles we just downscaled to our level to the list of tiles to copy
    tilesToCopy.insert(tilesToCopy.end(), perLevelTilesToDownscale[mipMapLevel].begin(), perLevelTilesToDownscale[mipMapLevel].end());
    
    // Finally copy with multiple threads each tile from the cache
    boost::scoped_ptr<CachePixelsTransferProcessorBase> processor;
    switch (bitdepth) {
        case eImageBitDepthByte:
            processor.reset(new CachePixelsTransferProcessor<false /*copyToCache*/, unsigned char>(renderClone));
            break;
        case eImageBitDepthShort:
            processor.reset(new CachePixelsTransferProcessor<false /*copyToCache*/, unsigned short>(renderClone));
            break;
        case eImageBitDepthFloat:
            processor.reset(new CachePixelsTransferProcessor<false /*copyToCache*/, float>(renderClone));
            break;
        default:
            break;
    }
    processor->setValues(this, tilesToCopy);
    ActionRetCodeEnum stat = processor->launchThreadsBlocking();


    // Release the tiles lock before calling updateCachedTilesStateMap which may try to take a write lock on an already taken read lock
    cacheDataDeleter.reset();

    // In persistent mode we have to actually copy the states map from the cache entry to the cache
    if (internalCacheEntry->isPersistent() && stateMapUpdated) {
        updateCachedTilesStateMap(tilesToUpdate, false);
    } // stateMapUpdated


    return stat;

} // fetchAndCopyCachedTiles

ActionRetCodeEnum
ImageCacheEntry::fetchCachedTilesAndUpdateStatus(bool readOnly, TileStateHeader* tileStatus, bool* hasUnRenderedTile, bool *hasPendingResults)
{
#if defined(TRACE_TILES_STATUS) || defined(TRACE_TILES_STATUS_SHORT)
    _imp->writeDebugStatus("fetchCachedTilesAndUpdateStatus", true);
#endif


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

        _imp->updateStateMapReadOnly = readOnly;

        if (_imp->cachePolicy == eCacheAccessModeNone) {
            // When not interacting with the cache, the internalCacheEntry is actually local to this object
            ImageCacheEntryPrivate::UpdateStateMapRetCodeEnum stat = _imp->readAndUpdateStateMap(true /*hasExlcusiveLock*/, false);
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
            _imp->internalCacheEntry->tileSizeX = _imp->localTilesState.tileSizeX;
            _imp->internalCacheEntry->tileSizeY = _imp->localTilesState.tileSizeY;

            CacheEntryLockerBasePtr cacheAccess = _imp->internalCacheEntry->getFromCache();

            CacheEntryLockerBase::CacheEntryStatusEnum cacheStatus = cacheAccess->getStatus();
            while (cacheStatus == CacheEntryLockerBase::eCacheEntryStatusComputationPending) {
                cacheStatus = cacheAccess->waitForPendingEntry();
            }
            assert(cacheStatus == CacheEntryLockerBase::eCacheEntryStatusCached ||
                   cacheStatus == CacheEntryLockerBase::eCacheEntryStatusMustCompute);

            bool mustClearCache = _imp->cachePolicy == eCacheAccessModeWriteOnly;
            if (mustClearCache && cacheStatus == CacheEntryLockerBase::eCacheEntryStatusCached) {
                _imp->internalCacheEntry->getCache()->removeEntry(_imp->internalCacheEntry);

                if (!appPTR->getTileCache()->isPersistent()) {
                    _imp->internalCacheEntry = ImageCacheEntryInternal<false>::create(_imp->key);
                } else {
                    _imp->internalCacheEntry = ImageCacheEntryInternal<true>::create(_imp.get(), _imp->key);
                }
                _imp->internalCacheEntry->tileSizeX = _imp->localTilesState.tileSizeX;
                _imp->internalCacheEntry->tileSizeY = _imp->localTilesState.tileSizeY;
                _imp->markedTiles.clear();
                _imp->tilesToFetch.clear();
                _imp->tilesToDownscale.clear();
                
                cacheAccess = _imp->internalCacheEntry->getFromCache();
                cacheStatus = cacheAccess->getStatus();
                while (cacheStatus == CacheEntryLockerBase::eCacheEntryStatusComputationPending) {
                    cacheStatus = cacheAccess->waitForPendingEntry();
                }
                assert(cacheStatus == CacheEntryLockerBase::eCacheEntryStatusCached ||
                       cacheStatus == CacheEntryLockerBase::eCacheEntryStatusMustCompute);


                const RectI bounds = _imp->localTilesState.state->bounds;
                _imp->localTilesState.init(_imp->localTilesState.tileSizeX, _imp->localTilesState.tileSizeY, bounds);


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
                    ImageCacheEntryPrivate::UpdateStateMapRetCodeEnum stat = _imp->readAndUpdateStateMap(false /*hasExlcusiveLock*/, false);
                    switch (stat) {
                        case ImageCacheEntryPrivate::eUpdateStateMapRetCodeUpToDate:
                            // We did not write anything on the state map, we are good
                            break;
                        case ImageCacheEntryPrivate::eUpdateStateMapRetCodeMustWriteToCache:
                            // We cannot be here: it is forbidden to write the state map under a read lock
                            assert(false);
                            break;
                        case ImageCacheEntryPrivate::eUpdateStateMapRetCodeNeedWriteLock:
                            if (!_imp->updateStateMapReadOnly) {
                                // We must update the state map but cannot do so under a read lock
                                mustCallUnderWriteLock = true;
                            }
                            break;
                        case ImageCacheEntryPrivate::eUpdateStateMapRetCodeFailed:
                            return eActionStatusFailed;

                    }

                }
                if (mustCallUnderWriteLock) {
                    boost::unique_lock<boost::shared_mutex> writeLock(nonPersistentLocalEntry->perMipMapTilesStateMutex);
                    ImageCacheEntryPrivate::UpdateStateMapRetCodeEnum stat = _imp->readAndUpdateStateMap(true /*hasExlcusiveLock*/, false);
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

                ImageCacheEntryPrivate::UpdateStateMapRetCodeEnum stat = _imp->readAndUpdateStateMap(true /*hasExclusiveLock*/, true);

                // All tiles should be eTileStatusNotRendered and thus we set them all to eTileStatusPending and must insert the results in te cache
                assert(stat == ImageCacheEntryPrivate::eUpdateStateMapRetCodeMustWriteToCache ||
                       stat == ImageCacheEntryPrivate::eUpdateStateMapRetCodeFailed ||
                       (_imp->updateStateMapReadOnly && stat == ImageCacheEntryPrivate::eUpdateStateMapRetCodeNeedWriteLock));
                (void)stat;
                if (!_imp->updateStateMapReadOnly) {
                    cacheAccess->insertInCache();
                }
            }

            if (!_imp->updateStateMapReadOnly && (!_imp->tilesToDownscale.empty() || !_imp->tilesToFetch.empty())) {
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
        // Make a copy of the internal state so it does not get modified by the caller
        tileStatus->init(_imp->localTilesState.tileSizeX, _imp->localTilesState.tileSizeY, _imp->localTilesState.state->bounds);
        *tileStatus->state = *_imp->localTilesState.state;
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
ImageCacheEntryPrivate::updateCachedTilesStateMap(const std::vector<TilesSet>& tilesToUpdate, bool updateAllTilesRegardless)
{
    assert(cachePolicy != eCacheAccessModeNone);
    if (!internalCacheEntry->isPersistent()) {
        return;
    }

#if defined(TRACE_TILES_STATUS) || defined(TRACE_TILES_STATUS_SHORT)
    writeDebugStatus("updateCachedTilesStateMap", true);
#endif


    ImageCacheEntryInternal<true>* persistentLocalEntry = dynamic_cast<ImageCacheEntryInternal<true>* >(internalCacheEntry.get());
    assert(persistentLocalEntry);

    // Re-lookup in the cache the entry, this will force a call to fromMemorySegment, thus updating the tiles state
    // as seen in the cache.
    // There may be a possibility that the tiles state were removed from the cache for some external reasons.
    // In this case we just re-insert back the entry in the cache.

    // Flag that we don't want to update the tile state locally
    persistentLocalEntry->tilesToUpdate = tilesToUpdate;
    persistentLocalEntry->updateAllTilesRegardless = updateAllTilesRegardless;
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

    bool didSomething = _imp->markCacheEntriesAsAbortedInternal();

    if (didSomething && _imp->cachePolicy != eCacheAccessModeNone) {
        // In persistent mode we have to actually copy the cache entry tiles state map to the cache
        if (_imp->internalCacheEntry->isPersistent()) {
            _imp->updateCachedTilesStateMap(_imp->markedTiles, false);
        }

    }

    _imp->markedTiles.clear();


} // markCacheTilesAsAborted

bool
ImageCacheEntryPrivate::markCacheEntriesAsAbortedInternal()
{

    assert(!lock.try_lock());

    if (markedTiles.empty()) {
        return false;
    }

#if defined(TRACE_TILES_STATUS) || defined(TRACE_TILES_STATUS_SHORT)
    writeDebugStatus("markCacheTilesAsAborted", true);
#endif


    boost::scoped_ptr<boost::unique_lock<boost::shared_mutex> > writeLock;
    if (!internalCacheEntry->isPersistent()) {
        // In non-persistent mode, lock the cache entry since it's shared across threads.
        // In persistent mode the entry is copied in fromMemorySegment
        ImageCacheEntryInternal<false>* nonPersistentLocalEntry = dynamic_cast<ImageCacheEntryInternal<false>* >(internalCacheEntry.get());
        assert(nonPersistentLocalEntry);
        writeLock.reset(new boost::unique_lock<boost::shared_mutex>(nonPersistentLocalEntry->perMipMapTilesStateMutex));
    }

    // We should have gotten the state map from the cache in fetchCachedTilesAndUpdateStatus()
    assert(!internalCacheEntry->perMipMapTilesState.empty());


    // Read the cache map and update our local map
    CacheBasePtr cache = internalCacheEntry->getCache();
    bool hasModifiedTileMap = false;

    for (std::size_t i = 0; i < markedTiles.size(); ++i) {

        if (internalCacheEntry->perMipMapTilesState[i].tiles.empty()) {
            continue;
        }
        TileStateHeader cacheStateMap = TileStateHeader(localTilesState.tileSizeX, localTilesState.tileSizeY, &internalCacheEntry->perMipMapTilesState[i]);


        for (TilesSet::iterator it = markedTiles[i].begin(); it != markedTiles[i].end(); ++it) {


            TileState* cacheTileState = cacheStateMap.getTileAt(it->tx, it->ty);


            // We marked the cache tile status to eTileStatusPending previously in
            // readAndUpdateStateMap
            // Mark it as eTileStatusNotRendered now
            assert(i != mipMapLevel || (cacheTileState->status == eTileStatusPending || cacheTileState->status == eTileStatusNotRendered));
            cacheTileState->status = eTileStatusNotRendered;
            hasModifiedTileMap = true;
#ifdef TRACE_TILES_STATUS
#ifdef TRACE_TILES_ONLY_PRINT_FIRST_TILE
            if (it->tx == 0 && it->ty == 0)
#endif
            qDebug() << QThread::currentThread() << debugId << "marking " << it->tx << it->ty << "unrendered at level" << i;
#endif
            if (i == mipMapLevel) {
                TileState* localTileState = localTilesState.getTileAt(it->tx, it->ty);
                assert(localTileState->status == eTileStatusNotRendered);
                if (localTileState->status == eTileStatusNotRendered) {

                    localTileState->status = eTileStatusNotRendered;
                }
            }
        }
    }


    if (!hasModifiedTileMap) {
        markedTiles.clear();
        return false;
    }

    return true;
} // markCacheEntriesAsAbortedInternal

void
ImageCacheEntry::markCacheTilesInRegionAsNotRendered(const RectI& roi)
{
    // Make sure to call fetchCachedTilesAndUpdateStatus() first
    assert(_imp->internalCacheEntry);
    if (roi.isNull()) {
        return;
    }
 
    RectI roiIntersected;
    roi.intersect(_imp->localTilesState.state->bounds, &roiIntersected);

    // Protect all local structures against multiple threads using this object.
    boost::unique_lock<boost::mutex> locker(_imp->lock);

#if defined(TRACE_TILES_STATUS) || defined(TRACE_TILES_STATUS_SHORT)
    _imp->writeDebugStatus("markCacheTilesInRegionAsNotRendered", true);
#endif

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

    std::vector<TileInternalIndex> tileIndicesToRelease;


    RectI mipmap0Roi = roiIntersected.upscalePowerOfTwo(_imp->mipMapLevel);


    for (std::size_t i = 0; i < _imp->internalCacheEntry->perMipMapTilesState.size(); ++i) {
        
        RectI levelRoi = mipmap0Roi.downscalePowerOfTwo(i);

        RectI levelRoIRounded = levelRoi;
        levelRoIRounded.roundToTileSize(_imp->localTilesState.tileSizeX, _imp->localTilesState.tileSizeY);
        TileStateHeader cacheStateMap = TileStateHeader(_imp->localTilesState.tileSizeX, _imp->localTilesState.tileSizeY, &_imp->internalCacheEntry->perMipMapTilesState[i]);
        if (cacheStateMap.state->tiles.empty()) {
            continue;
        }

        for (int ty = levelRoIRounded.y1; ty < levelRoIRounded.y2; ty += _imp->localTilesState.tileSizeY) {
            for (int tx = levelRoIRounded.x1; tx < levelRoIRounded.x2; tx += _imp->localTilesState.tileSizeX) {

                assert(tx % _imp->localTilesState.tileSizeX == 0 && ty % _imp->localTilesState.tileSizeY == 0);

                if (i == _imp->mipMapLevel) {
                    TileState* localTileState = _imp->localTilesState.getTileAt(tx, ty);
                    assert(localTileState);
                    localTileState->status = eTileStatusNotRendered;
                }

                TileState* cacheTileState = cacheStateMap.getTileAt(tx, ty);
                assert(cacheTileState);
                if (cacheTileState->status == eTileStatusNotRendered) {
                    continue;
                }
                cacheTileState->status = eTileStatusNotRendered;
                hasModifiedTileMap = true;
#ifdef TRACE_TILES_STATUS
#ifdef TRACE_TILES_ONLY_PRINT_FIRST_TILE
                if (tx == 0 && ty == 0)
#endif
                    qDebug() << QThread::currentThread() << _imp->effect.lock().get() << _imp->effect.lock()->getScriptName_mt_safe().c_str() << _imp->internalCacheEntry->getHashKey() << "marking " << tx << ty << "unrendered at level" << i;
#endif

                for (int c = 0; c < _imp->nComps; ++c) {
                    tileIndicesToRelease.push_back(cacheTileState->channelsTileStorageIndex[c]);
                }
            }
        }
    }


    if (!tileIndicesToRelease.empty() && _imp->cachePolicy != eCacheAccessModeNone) {
        _imp->internalCacheEntry->getCache()->releaseTiles(_imp->internalCacheEntry, tileIndicesToRelease);
    }


    if (_imp->cachePolicy != eCacheAccessModeNone && hasModifiedTileMap) {
        // In persistent mode we have to actually copy the cache entry tiles state map to the cache
        if (_imp->internalCacheEntry->isPersistent()) {
            _imp->updateCachedTilesStateMap(std::vector<TilesSet>(), true);
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

#if defined(TRACE_TILES_STATUS) || defined(TRACE_TILES_STATUS_SHORT)
    _imp->writeDebugStatus("markCacheTilesAsRendered", true);
#endif

    std::vector<boost::shared_ptr<TileData> > tilesToCopy;

    {

        TileStateHeader cacheStateMap = TileStateHeader(_imp->localTilesState.tileSizeX, _imp->localTilesState.tileSizeY, &_imp->internalCacheEntry->perMipMapTilesState[_imp->mipMapLevel]);

        for (TilesSet::iterator it = _imp->markedTiles[_imp->mipMapLevel].begin(); it != _imp->markedTiles[_imp->mipMapLevel].end(); ++it) {

            TileState* cacheTileState = cacheStateMap.getTileAt(it->tx, it->ty);

            // We marked the cache tile status to eTileStatusPending previously in
            // readAndUpdateStateMap
            // Mark it as eTileStatusRendered now
            assert(cacheTileState->status == eTileStatusPending);
            cacheTileState->status = _imp->isDraftModeEnabled ? eTileStatusRenderedLowQuality : eTileStatusRenderedHighestQuality;

#ifdef TRACE_TILES_STATUS
#ifdef TRACE_TILES_ONLY_PRINT_FIRST_TILE
            if (it->tx == 0 && it->ty == 0)
#endif
            qDebug() << QThread::currentThread() << _imp->effect.lock().get()  << _imp->effect.lock()->getScriptName_mt_safe().c_str() << _imp->image.lock()->getLayer().getPlaneLabel().c_str() << _imp->internalCacheEntry->getHashKey() <<  "marking " << it->tx << it->ty << "rendered at level" << _imp->mipMapLevel;
#endif
            hasModifiedTileMap = true;


            TileState* localTileState = _imp->localTilesState.getTileAt(it->tx, it->ty);
            assert(localTileState->status == eTileStatusNotRendered);
            if (localTileState->status == eTileStatusNotRendered) {

                localTileState->status = cacheTileState->status;

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

    std::vector<TilesSet> tilesToUpdate = _imp->markedTiles;
    _imp->markedTiles.clear();

#ifdef DEBUG
    // Check that all tiles are marked either rendered or pending
    RectI roiRounded = _imp->roi;
    roiRounded.roundToTileSize(_imp->localTilesState.tileSizeX, _imp->localTilesState.tileSizeY);
    for (int ty = roiRounded.y1; ty < roiRounded.y2; ty += _imp->localTilesState.tileSizeY) {
        for (int tx = roiRounded.x1; tx < roiRounded.x2; tx += _imp->localTilesState.tileSizeX) {

            assert(tx % _imp->localTilesState.tileSizeX == 0 && ty % _imp->localTilesState.tileSizeY == 0);
            TileState* localTileState = _imp->localTilesState.getTileAt(tx, ty);
            assert(localTileState->status == eTileStatusPending || localTileState->status == eTileStatusRenderedHighestQuality || localTileState->status == eTileStatusRenderedLowQuality);
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


#ifdef NATRON_CACHE_TILES_MEMORY_ALLOCATOR_CENTRALIZED
    std::size_t nTilesToAlloc = tilesToCopy.size();
#else
    U64 entryHash = _imp->internalCacheEntry->getHashKey();
    std::vector<TileHash> tilesAllocNeeded(tilesToCopy.size());
    for (std::size_t i = 0; i < tilesToCopy.size(); ++i) {
        tilesAllocNeeded[i] = CacheBase::makeTileCacheIndex(tilesToCopy[i]->bounds.x1, tilesToCopy[i]->bounds.y1, _imp->mipMapLevel, tilesToCopy[i]->channel_i, entryHash);
    }
#endif


    // Allocate buffers for tiles
    std::vector<std::pair<TileInternalIndex, void*> > allocatedTiles;
    void* cacheData;
    bool gotTiles = cache->retrieveAndLockTiles(_imp->internalCacheEntry, 0 /*existingTiles*/,
#ifdef NATRON_CACHE_TILES_MEMORY_ALLOCATOR_CENTRALIZED
                                                nTilesToAlloc,
#else
                                                &tilesAllocNeeded,
#endif
                                                NULL, &allocatedTiles, &cacheData);


    boost::scoped_ptr<CacheDataLock_RAII> cacheDataDeleter(new CacheDataLock_RAII(cache, cacheData));
    if (!gotTiles) {
        return;
    }

    // This is the tiles state at the mipmap level of interest in the cache
    TileStateHeader cacheStateMap(_imp->localTilesState.tileSizeX, _imp->localTilesState.tileSizeY, &_imp->internalCacheEntry->perMipMapTilesState[_imp->mipMapLevel]);
    assert(!cacheStateMap.state->tiles.empty());

    // Set the tile pointer to the tiles to copy
    for (std::size_t i = 0; i < tilesToCopy.size(); ++i) {
        tilesToCopy[i]->ptr = allocatedTiles[i].second;

        tilesToCopy[i]->tileCache_i = allocatedTiles[i].first;
#ifdef DEBUG
        assert(cache->checkTileIndex(tilesToCopy[i]->tileCache_i));
#endif
        // update the tile indices
        int tx = (int)std::floor((double)tilesToCopy[i]->bounds.x1 / _imp->localTilesState.tileSizeX) * _imp->localTilesState.tileSizeX;
        int ty = (int)std::floor((double)tilesToCopy[i]->bounds.y1 / _imp->localTilesState.tileSizeY) * _imp->localTilesState.tileSizeY;
        TileState* cacheTileState = cacheStateMap.getTileAt(tx, ty);
        //assert(allocatedTiles[i].first.index != (U64)-1);
        cacheTileState->channelsTileStorageIndex[tilesToCopy[i]->channel_i] = allocatedTiles[i].first;

        TileState* localTileState = _imp->localTilesState.getTileAt(tx, ty);
        localTileState->channelsTileStorageIndex[tilesToCopy[i]->channel_i] = allocatedTiles[i].first;
    }

    EffectInstancePtr renderClone = _imp->effect.lock();

    // Finally copy over multiple threads each tile
    boost::scoped_ptr<CachePixelsTransferProcessorBase> processor;
    switch (_imp->bitdepth) {
        case eImageBitDepthByte:
            processor.reset(new CachePixelsTransferProcessor<true /*copyToCache*/, unsigned char>(renderClone));
            break;
        case eImageBitDepthShort:
            processor.reset(new CachePixelsTransferProcessor<true /*copyToCache*/, unsigned short>(renderClone));
            break;
        case eImageBitDepthFloat:
            processor.reset(new CachePixelsTransferProcessor<true /*copyToCache*/, float>(renderClone));
            break;
        default:
            break;
    }

    processor->setValues(_imp.get(), tilesToCopy);
    ActionRetCodeEnum stat = processor->launchThreadsBlocking();

    // We never abort when copying tiles to the cache since they are anyway already rendered.
    assert(stat == eActionStatusOK);
    (void)stat;

    // We must delete the CacheDataLock_RAII now because updateCachedTilesStateMap may attempt to get a write lock on an already taken read lock

    cacheDataDeleter.reset();

    // In persistent mode we have to actually copy the cache entry tiles state map to the cache
    if (_imp->internalCacheEntry->isPersistent()) {
        _imp->updateCachedTilesStateMap(tilesToUpdate, false);
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

#if defined(TRACE_TILES_STATUS) || defined(TRACE_TILES_STATUS_SHORT)
    _imp->writeDebugStatus("waitForPendingTiles", true);
#endif


    // When the cache is persistent, we don't hold a lock on the entry since it would also require locking
    // some mutexes protecting the memory mapping of the cache itself.
    //
    // For more explanation see comments in CacheEntryLocker::waitForPendingEntry:
    // Instead we implement polling

    // If this thread is a threadpool thread, it may wait for a while that results gets available.
    // Release the thread to the thread pool so that it may use this thread for other runnables
    // and reserve it back when done waiting.
    RELEASE_THREAD_RAII();

    std::size_t timeSpentWaitingForPendingEntryMS = 0;
    std::size_t timeToWaitMS = 40;


    bool hasUnrenderedTile;
    bool hasPendingResults;

    do {
        hasUnrenderedTile = false;
        hasPendingResults = false;
        ActionRetCodeEnum stat = fetchCachedTilesAndUpdateStatus(false, NULL, &hasUnrenderedTile, &hasPendingResults);
        if (isFailureRetCode(stat)) {
            return true;
        }

        if (hasPendingResults) {

            timeSpentWaitingForPendingEntryMS += timeToWaitMS;
            CacheEntryLockerBase::sleep_milliseconds(timeToWaitMS);

            // Increase the time to wait at the next iteration
            timeToWaitMS *= 1.2;


        }
#ifdef DEBUG
        if (timeSpentWaitingForPendingEntryMS > 5000) {
            qDebug() << "WARNING:" << _imp->effect.lock()->getScriptName_mt_safe().c_str() << "stuck in waitForPendingTiles() for more than " << Timer::printAsTime(timeSpentWaitingForPendingEntryMS / 1000, false);
        }
#endif

    } while(hasPendingResults && !hasUnrenderedTile && !_imp->effect.lock()->isRenderAborted());

#if defined(TRACE_TILES_STATUS) || defined(TRACE_TILES_STATUS_SHORT)
    _imp->writeDebugStatus("waitForPendingTiles", false);
#endif


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

static std::string getUUIDPropName(unsigned int mipMapLevel)
{
    return getPropNameInternal("UUID", mipMapLevel);
}

/**
 * @brief Read the tiles state map for each mipmap level from the cache properties
 **/
static CacheEntryBase::FromMemorySegmentRetCodeEnum fromMemorySegmentInternal(int tileSizeX, int tileSizeY,
                                                                              const IPCPropertyMap& properties,
                                                                              std::vector<TilesState>* localMipMapStates)
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
        std::string uuidPropName = getUUIDPropName(m);

        const IPCProperty* statusProp = properties.getIPCProperty(statusPropName);
        const IPCProperty* indicesProp = properties.getIPCProperty(tileIndicesPropName);
        const IPCProperty* boundsProp = properties.getIPCProperty(boundsPropName);
        const IPCProperty* uuidProp = properties.getIPCProperty(uuidPropName);

        if (!statusProp || statusProp->getType() != eIPCVariantTypeInt) {
            return CacheEntryBase::eFromMemorySegmentRetCodeFailed;
        }

        if (!indicesProp || indicesProp->getType() != eIPCVariantTypeULongLong) {
            return CacheEntryBase::eFromMemorySegmentRetCodeFailed;
        }

        if (!uuidProp || uuidProp->getType() != eIPCVariantTypeULongLong || uuidProp->getNumDimensions() != statusProp->getNumDimensions() * 2) {
            return CacheEntryBase::eFromMemorySegmentRetCodeFailed;
        }

        if (!boundsProp || boundsProp->getType() != eIPCVariantTypeInt || boundsProp->getNumDimensions() != 4) {
            return CacheEntryBase::eFromMemorySegmentRetCodeFailed;
        }

        if (indicesProp->getNumDimensions() != statusProp->getNumDimensions() * 4) {
            return CacheEntryBase::eFromMemorySegmentRetCodeFailed;
        }

        IPCProperty::getIntValue(boundsProp->getData(), 0, &localState.bounds.x1);
        IPCProperty::getIntValue(boundsProp->getData(), 1, &localState.bounds.y1);
        IPCProperty::getIntValue(boundsProp->getData(), 2, &localState.bounds.x2);
        IPCProperty::getIntValue(boundsProp->getData(), 3, &localState.bounds.y2);


        localState.boundsRoundedToTileSize = localState.bounds;
        localState.boundsRoundedToTileSize.roundToTileSize(tileSizeX, tileSizeY);

        localState.tiles.resize(statusProp->getNumDimensions());

        // Ensure the number of tiles is coherent
        assert((int)localState.tiles.size() == ((localState.boundsRoundedToTileSize.width() / tileSizeX) * (localState.boundsRoundedToTileSize.height() / tileSizeY)));

        int tx = localState.boundsRoundedToTileSize.x1;
        int ty = localState.boundsRoundedToTileSize.y1;
        for (std::size_t i = 0; i < localState.tiles.size(); ++i) {
            TileState& state = localState.tiles[i];

            int status_i;
            IPCProperty::getIntValue(statusProp->getData(), i, &status_i);
            state.status = (TileStatusEnum)status_i;

            // The UUID is a 128-bit value that can be encoded with 2 unsigned long long
            U64 encodedUUID[2];
            IPCProperty::getULongLongValue(uuidProp->getData(), i * 2, &encodedUUID[0]);
            IPCProperty::getULongLongValue(uuidProp->getData(), i * 2 + 1, &encodedUUID[1]);
            assert(sizeof(U64) * 2 == 16);
            // See http://www.boost.org/doc/libs/1_63_0/libs/uuid/uuid.html
            // A boost::uuids::uuid is exactly 16 bytes
            memcpy(&state.uuid, encodedUUID, 16);


            for (int c = 0; c < 4; ++c) {
                TileInternalIndexU64Converter converter;
                IPCProperty::getULongLongValue(indicesProp->getData(), i * 4 + c, &converter.raw);
                state.channelsTileStorageIndex[c] = converter.index;
            }

            assert(tx < localState.boundsRoundedToTileSize.x2);
            assert(ty < localState.boundsRoundedToTileSize.y2);

            state.bounds.x1 = std::max(localState.bounds.x1, tx);
            state.bounds.y1 = std::max(localState.bounds.y1, ty);
            state.bounds.x2 = std::min(localState.bounds.x2, tx + tileSizeX);
            state.bounds.y2 = std::min(localState.bounds.y2, ty + tileSizeY);

            tx += tileSizeX;
            if (tx >= localState.boundsRoundedToTileSize.x2) {
                tx = localState.boundsRoundedToTileSize.x1;
                ty += tileSizeY;
            }
        }
    }
    return CacheEntryBase::eFromMemorySegmentRetCodeOk;
} // fromMemorySegmentInternal

#ifdef TRACE_TILES_STATUS
std::string getTileStatusString(TileStatusEnum s)
{
    switch (s) {
        case eTileStatusPending:
            return "pending";
        case eTileStatusNotRendered:
            return "not rendered";
        case eTileStatusRenderedLowQuality:
            return "rendered low quality";
        case eTileStatusRenderedHighestQuality:
            return "rendered high quality";
    }
}
#endif

/**
 * @brief The implementation that copies the tile status from the local state map to the cache state map
 * @param copyPendingStatusToCache If true then we blindly copy the status to the cache. If false, we only
 * update tiles that are locally marked rendered or not rendered.
 **/
static void toMemorySegmentInternal(bool copyPendingStatusToCache,
                                    const std::vector<TilesState>& localMipMapStates,
                                    IPCPropertyMap* properties,
                                    const ImageCacheEntryInternal<true>* persistentEntry)
{

    const int numLevels = (int)localMipMapStates.size();

    {
        IPCProperty* numLevelsProp = properties->getOrCreateIPCProperty("NumLevels", eIPCVariantTypeInt);
        int currentNumLevels = 0;
        int levelPropNDims = numLevelsProp->getNumDimensions();
        if (numLevelsProp && levelPropNDims > 0) {
            IPCProperty::getIntValue(*numLevelsProp->getData(), 0, &currentNumLevels);
        }
        if (levelPropNDims == 0) {
            numLevelsProp->resize(1);
        }
        if (currentNumLevels < numLevels) {
            IPCProperty::setIntValue(0, numLevels, numLevelsProp->getData());
        }
    }
    for (std::size_t m = 0; m < localMipMapStates.size(); ++m) {

        // Update each mipmap level
        const TilesState& mipmapState = localMipMapStates[m];

        std::string statusPropName = getStatusPropName(m);
        std::string tileIndicesPropName = getTileIndicesPropName(m);
        std::string boundsPropName = getBoundsPropName(m);
        std::string uuidPropName = getUUIDPropName(m);

        IPCProperty* statusProp = properties->getOrCreateIPCProperty(statusPropName, eIPCVariantTypeInt);
        IPCProperty* indicesProp = properties->getOrCreateIPCProperty(tileIndicesPropName, eIPCVariantTypeULongLong);
        IPCProperty* uuidProp = properties->getOrCreateIPCProperty(uuidPropName, eIPCVariantTypeULongLong);
        IPCProperty* boundsProp = properties->getOrCreateIPCProperty(boundsPropName, eIPCVariantTypeInt);

        // If the properties do not have the appropriate size, resize them

        if (statusProp->getNumDimensions() != mipmapState.tiles.size() ||
            indicesProp->getNumDimensions() != mipmapState.tiles.size() * 4 ||
            uuidProp->getNumDimensions() != mipmapState.tiles.size() * 2 ||
            boundsProp->getNumDimensions() != 4) {

            statusProp->resize(mipmapState.tiles.size());

            // Each tile has up to 4 channels allocated in the cache
            indicesProp->resize(mipmapState.tiles.size() * 4);

            // Each tile has a uuid that takes 2 U64 in memory (16 bytes)
            uuidProp->resize(mipmapState.tiles.size() * 2);

            // The bounds of the this mipmap level tiles state map
            boundsProp->resize(4);

            IPCProperty::setIntValue(0, mipmapState.bounds.x1, boundsProp->getData());
            IPCProperty::setIntValue(1, mipmapState.bounds.y1, boundsProp->getData());
            IPCProperty::setIntValue(2, mipmapState.bounds.x2, boundsProp->getData());
            IPCProperty::setIntValue(3, mipmapState.bounds.y2, boundsProp->getData());

            for (std::size_t i = 0; i < mipmapState.tiles.size(); ++i) {

                IPCProperty::setIntValue(i, (int)mipmapState.tiles[i].status, statusProp->getData());
#ifdef TRACE_TILES_STATUS
#ifdef TRACE_TILES_ONLY_PRINT_FIRST_TILE
                if (mipmapState.tiles[i].bounds.x1 == 0 && mipmapState.tiles[i].bounds.y1 == 0)
#endif
                qDebug() << QThread::currentThread() << persistentEntry->_imp->debugId << "initializing status in cache of " << mipmapState.tiles[i].bounds.x1 << mipmapState.tiles[i].bounds.y1 << "at level" << m << "to" << getTileStatusString(mipmapState.tiles[i].status).c_str();
#endif

                U64 encodedUUID[2];
                // See http://www.boost.org/doc/libs/1_63_0/libs/uuid/uuid.html
                // A boost::uuids::uuid is exactly 16 bytes
                memcpy(encodedUUID, &mipmapState.tiles[i].uuid, 16);
                IPCProperty::setULongLongValue(i * 2, encodedUUID[0], uuidProp->getData());
                IPCProperty::setULongLongValue(i * 2 + 1, encodedUUID[1], uuidProp->getData());

                for (int c = 0; c < 4; ++c) {
                    TileInternalIndexU64Converter converter;
                    converter.index = mipmapState.tiles[i].channelsTileStorageIndex[c];
                    IPCProperty::setULongLongValue(i * 4 + c, converter.raw, indicesProp->getData());
                }

            }

        } else {
            assert(statusProp->getNumDimensions() == mipmapState.tiles.size() &&
                   indicesProp->getNumDimensions() == mipmapState.tiles.size() * 4 &&
                   uuidProp->getNumDimensions() == mipmapState.tiles.size() * 2);


            assert(boundsProp->getNumDimensions() == 4);
#ifdef DEBUG
            // Ensure the bounds in the cache correspond to the local bounds
            RectI boundsCheck;
            IPCProperty::getIntValue(*boundsProp->getData(), 0, &boundsCheck.x1);
            IPCProperty::getIntValue(*boundsProp->getData(), 1, &boundsCheck.y1);
            IPCProperty::getIntValue(*boundsProp->getData(), 2, &boundsCheck.x2);
            IPCProperty::getIntValue(*boundsProp->getData(), 3, &boundsCheck.y2);
            assert(boundsCheck == mipmapState.bounds);
#endif
            // Check the number of tiles is consistent with the bounds
            int nTilesPerRow = mipmapState.boundsRoundedToTileSize.width() / persistentEntry->_imp->localTilesState.tileSizeX;
            assert((int)mipmapState.tiles.size() == (nTilesPerRow *(mipmapState.boundsRoundedToTileSize.height() / persistentEntry->_imp->localTilesState.tileSizeY)));

            for (std::size_t i = 0; i < mipmapState.tiles.size(); ++i) {

                int cacheStatus_i;
                {
                    IPCProperty::getIntValue(*statusProp->getData(), i, &cacheStatus_i);
                    // If the tile in the cache is already marked rendered at highest quality, do not update the state.
                    // If the tile is marked low quality in the cache and we did not render a higher quality, do not update it either
                    if (!copyPendingStatusToCache && ((TileStatusEnum)cacheStatus_i == eTileStatusRenderedHighestQuality ||
                        ((TileStatusEnum)cacheStatus_i == eTileStatusRenderedLowQuality && mipmapState.tiles[i].status != eTileStatusRenderedHighestQuality))) {
                        continue;
                    }

                    if (!persistentEntry->updateAllTilesRegardless) {
                        // Check if we own the tile
                        if (m >= persistentEntry->tilesToUpdate.size()) {
                            continue;
                        }
                        int tile_x = i % nTilesPerRow;
                        int tile_y = i / nTilesPerRow;

                        TileCoord coord = {
                            mipmapState.boundsRoundedToTileSize.x1 + tile_x * persistentEntry->_imp->localTilesState.tileSizeX,
                            mipmapState.boundsRoundedToTileSize.y1 + tile_y * persistentEntry->_imp->localTilesState.tileSizeY};
                        if (persistentEntry->tilesToUpdate[m].find(coord) == persistentEntry->tilesToUpdate[m].end()) {
                            continue;
                        }
                    }
                }

                // We specifically do not want to push the "pending" state to the cache
                if (copyPendingStatusToCache || mipmapState.tiles[i].status != eTileStatusPending) {

#ifdef TRACE_TILES_STATUS
                    if ((int)mipmapState.tiles[i].status != cacheStatus_i) {
#ifdef TRACE_TILES_ONLY_PRINT_FIRST_TILE
                        if (mipmapState.tiles[i].bounds.x1 == 0 && mipmapState.tiles[i].bounds.y1 == 0)
#endif
                        qDebug() << QThread::currentThread() << persistentEntry->_imp->debugId <<  "updating status in cache of " << mipmapState.tiles[i].bounds.x1 << mipmapState.tiles[i].bounds.y1 << "at level" << m << "to" << getTileStatusString(mipmapState.tiles[i].status).c_str() << "(from" << getTileStatusString((TileStatusEnum)cacheStatus_i).c_str() << ")";
                    }
#endif

                    U64 encodedUUID[2];
                    // See http://www.boost.org/doc/libs/1_63_0/libs/uuid/uuid.html
                    // A boost::uuids::uuid is exactly 16 bytes
                    memcpy(encodedUUID, &mipmapState.tiles[i].uuid, 16);
                    IPCProperty::setULongLongValue(i * 2, encodedUUID[0], uuidProp->getData());
                    IPCProperty::setULongLongValue(i * 2 + 1, encodedUUID[1], uuidProp->getData());


                    IPCProperty::setIntValue(i, (int)mipmapState.tiles[i].status, statusProp->getData());
                    for (int c = 0; c < 4; ++c) {
                        TileInternalIndexU64Converter converter;
                        converter.index = mipmapState.tiles[i].channelsTileStorageIndex[c];
                        IPCProperty::setULongLongValue(i * 4 + c, converter.raw, indicesProp->getData());
                    }
#if 0
                    if (mipmapState.tiles[i].status == eTileStatusRenderedHighestQuality || mipmapState.tiles[i].status == eTileStatusRenderedLowQuality) {
                        bool hasValidTile = false;
                        for (int c = 0; c < 4; ++c) {
                            if (mipmapState.tiles[i].channelsTileStorageIndex[c].index != (U64)-1) {
                                hasValidTile = true;
                                break;
                            }
                        }
                        assert(hasValidTile);
                    }
#endif
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
            toMemorySegmentInternal(false /*copyPendingStatusToCache*/, perMipMapTilesState, hackedProps, persistentEntry);
        } else {

            // Copy the tiles states locally
            fromMemorySegmentInternal(tileSizeX, tileSizeY, properties, &perMipMapTilesState);

            // Read the state map: it might modify it by marking tiles pending
            ImageCacheEntryPrivate::UpdateStateMapRetCodeEnum stat = persistentEntry->_imp->readAndUpdateStateMap(isLockedForWriting, false);
            switch (stat) {
                case ImageCacheEntryPrivate::eUpdateStateMapRetCodeUpToDate:
                    // We did not write anything on the state map, we are good
                    break;
                case ImageCacheEntryPrivate::eUpdateStateMapRetCodeMustWriteToCache: {
                    // We have written the state map, we must update it
                    assert(isLockedForWriting);
                    IPCPropertyMap* hackedProps = const_cast<IPCPropertyMap*>(&properties);
                    toMemorySegmentInternal(true /*copyPendingStatusToCache*/, perMipMapTilesState, hackedProps, persistentEntry);

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
    const ImageCacheEntryInternal<true>* persistentEntry = dynamic_cast<const ImageCacheEntryInternal<true>*>(this);
    toMemorySegmentInternal(true /*copyPendingStatusToCache*/, perMipMapTilesState, properties, persistentEntry);
    CacheEntryBase::toMemorySegment(properties);
} // toMemorySegment


NATRON_NAMESPACE_EXIT
