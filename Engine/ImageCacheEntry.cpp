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

#include "Engine/AppManager.h"
#include "Engine/Cache.h"
#include "Engine/ImageStorage.h"
#include "Engine/ImagePrivate.h"
#include "Engine/ImageTilesState.h"
#include "Engine/MultiThread.h"

NATRON_NAMESPACE_ENTER;

typedef std::set<TileCoord, TileCoord_Compare> FetchedTilesSet;

struct ChannelData
{
    ImageTilesStatePtr state;

};

struct ImageCacheEntryPrivate
{

    ImageCacheEntry* _publicInterface;
    ImageBitDepthEnum bitdepth;
    RectI roi, roiRoundedToTileSize;
    std::vector<ChannelData> channels;
    ImageStorageBasePtr storage;
    EffectInstancePtr effect;

    ImageCacheEntryPrivate(ImageCacheEntry* publicInterface,
                           const RectI& pixelRod,
                           const RectI& roi,
                           ImageBitDepthEnum depth,
                           int nComps,
                           const ImageStorageBasePtr& storage,
                           const EffectInstancePtr& effect)
    : _publicInterface(publicInterface)
    , bitdepth(depth)
    , roi(roi)
    , roiRoundedToTileSize(roi)
    , channels(nComps)
    , storage(storage)
    , effect(effect)
    {
        assert(nComps > 0);
        int tileSizeX, tileSizeY;
        Cache::getTileSizePx(depth, &tileSizeX, &tileSizeY);
        roiRoundedToTileSize.roundToTileSize(tileSizeX, tileSizeY);
        for (int c = 0; c < nComps; ++c) {
            channels[c].state.reset(new ImageTilesState(pixelRod, tileSizeX, tileSizeY));
        }
    }

    void fetchAndCopyCachedTiles(const std::vector<TileStateMap>& newChannelsState);
};

ImageCacheEntry::ImageCacheEntry(const RectI& pixelRod,
                                 const RectI& roi,
                                 ImageBitDepthEnum depth,
                                 int nComps,
                                 const ImageStorageBasePtr& storage,
                                 const EffectInstancePtr& effect)
: CacheEntryBase(appPTR->getTileCache())
, _imp(new ImageCacheEntryPrivate(this, pixelRod, roi, depth, nComps, storage, effect))
{
}

ImageCacheEntry::~ImageCacheEntry()
{

}

std::size_t
ImageCacheEntry::getNumTiles() const
{
    int tileSizeX, tileSizeY;
    _imp->channels[0].state->getTileSize(&tileSizeX, &tileSizeY);
    const RectI& boundsRounded = _imp->roiRoundedToTileSize;
    assert(boundsRounded.x1 % tileSizeX && boundsRounded.y1 % tileSizeY &&
           boundsRounded.x2 % tileSizeX && boundsRounded.y2 % tileSizeY);
    int nWidth =  boundsRounded.width() / tileSizeX;
    int nHeight =  boundsRounded.height() / tileSizeY;
    return nWidth * (std::size_t)nHeight;
}



template <bool copyToCache>
class CachePixelsTransferProcessor : public MultiThreadProcessorBase
{

public:

    struct CopyData
    {
        Image::CPUData tileData;
        Image::CopyPixelsArgs cpyArgs;
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

    virtual ActionRetCodeEnum launchThreads(unsigned int nCPUs = 0) OVERRIDE FINAL WARN_UNUSED_RETURN
    {
        return MultiThreadProcessorBase::launchThreads(nCPUs);
    }

    

    virtual ActionRetCodeEnum multiThreadFunction(unsigned int threadID,
                                                  unsigned int nThreads) OVERRIDE FINAL WARN_UNUSED_RETURN
    {

        int fromIndex, toIndex;
        ImageMultiThreadProcessorBase::getThreadRange(threadID, nThreads, 0, _tileBuffers.size(), &fromIndex, &toIndex);
        for (int i = fromIndex; i < toIndex; ++i) {

            const Image::CPUData& tileBufData = _tileBuffers[i];

            RectI renderWindow = _localBuffer.bounds;
            tileBufData.bounds.intersect(_localBuffer.bounds, &renderWindow);

            // This function is very optimized and templated for most common cases
            // In the best optimized case, memcpy is used
            if (copyToCache) {
                ImagePrivate::convertCPUImage(renderWindow,
                                              eViewerColorSpaceLinear,
                                              eViewerColorSpaceLinear,
                                              false,
                                              0,
                                              Image::eAlphaChannelHandlingFillFromChannel,
                                              Image::eMonoToPackedConversionCopyToChannelAndLeaveOthers,
                                              (const void**)_localBuffer.ptrs,
                                              _localBuffer.nComps,
                                              _localBuffer.bitDepth,
                                              _localBuffer.bounds,
                                              (void**)tileBufData.ptrs,
                                              tileBufData.nComps,
                                              tileBufData.bitDepth,
                                              tileBufData.bounds,
                                              _effect);

            } else {
                ImagePrivate::convertCPUImage(renderWindow,
                                              eViewerColorSpaceLinear,
                                              eViewerColorSpaceLinear,
                                              false,
                                              0,
                                              Image::eAlphaChannelHandlingFillFromChannel,
                                              Image::eMonoToPackedConversionCopyToChannelAndLeaveOthers,
                                              (const void**)tileBufData.ptrs,
                                              tileBufData.nComps,
                                              tileBufData.bitDepth,
                                              tileBufData.bounds,
                                              (void**)_localBuffer.ptrs,
                                              _localBuffer.nComps,
                                              _localBuffer.bitDepth,
                                              _localBuffer.bounds,
                                              _effect);

            }

        }
        return eActionStatusOK;
    } // multiThreadFunction
};

typedef CachePixelsTransferProcessor<true> ToCachePixelsTransferProcessor;
typedef CachePixelsTransferProcessor<false> FromCachePixelsTransferProcessor;

struct FetchTile
{
    int index;
    TileCoord coord;

    FetchTile() : index(-1), coord() {}
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

void
ImageCacheEntryPrivate::fetchAndCopyCachedTiles(const std::vector<TileStateMap>& newChannelsState)
{
    // This is the current state of our image, update it according to the stateMap read from the cache

    // We can only transfer to the cache from a RAM buffer
    RAMImageStoragePtr storageIsRAM = toRAMImageStorage(storage);
    assert(storageIsRAM);

    int tileSizeX, tileSizeY;
    channels[0].state->getTileSize(&tileSizeX, &tileSizeY);

    // The image data we want to copy from
    Image::CPUData toData;
    {
        toData.bitDepth = bitdepth;
        toData.nComps = (int)channels.size();
        toData.bounds = roi;
        memset(toData.ptrs, 0, sizeof(void*) * 4);
        toData.ptrs[0] = storageIsRAM->getData();
    }

    std::vector<const TileStateMap*> currentChannelsState(channels.size());
    for (std::size_t i = 0; i < channels.size(); ++i) {
        currentChannelsState[i] = &channels[i].state->getTilesMap();
    }

    std::vector<std::vector<FetchTile> > perChannelTilesToFetch(channels.size());

    // For each tile in the RoI (rounded to the tile size):
    // Check the tile status, only copy from the cache if rendered
    for (int k = 0; k < (int)channels.size(); ++k) {
        for (int ty = roiRoundedToTileSize.y1; ty < roiRoundedToTileSize.y2; ty += tileSizeY) {
            for (int tx = roiRoundedToTileSize.x1; tx < roiRoundedToTileSize.x2; tx += tileSizeX) {

                assert(tx % tileSizeX == 0 && ty % tileSizeY == 0);

                TileCoord c = {tx, ty};

                // Find this tile in the current state. If already cached don't bother
                TileStateMap::const_iterator foundInOldMap = currentChannelsState[k]->find(c);
                assert(foundInOldMap != currentChannelsState[k]->end());
                if (foundInOldMap == currentChannelsState[k]->end()) {
                    continue;
                }
                if (foundInOldMap->second.status == eTileStatusRendered) {
                    continue;
                }

                // Ok not yet copied, now find in the new state map
                TileStateMap::const_iterator foundInNewMap = newChannelsState[k].find(c);
                assert(foundInNewMap != newChannelsState[k].end());
                if (foundInNewMap == newChannelsState[k].end()) {
                    continue;
                }
                if (foundInNewMap->second.status != eTileStatusRendered) {
                    continue;
                }


                FetchTile fetchData;
                fetchData.coord = c;
                fetchData.index = foundInNewMap->second.tiledStorageIndex;
                perChannelTilesToFetch[k].push_back(fetchData);
            }
            
        }
    }
    
    // Set the current tile map to be the new one
    for (std::size_t i = 0; i < channels.size(); ++i) {
        channels[i].state->setTilesMap(newChannelsState[i]);
    }


    // Get a vector of tile indices to fetch from the cache directly
    std::vector<int> tileIndicesToFetch;
    for (std::size_t c = 0; c < perChannelTilesToFetch.size(); ++c) {
        for (std::size_t i = 0; i < perChannelTilesToFetch[c].size(); ++i) {
            tileIndicesToFetch.push_back(perChannelTilesToFetch[c][i].index);
        }
    }


    CachePtr tileCache = _publicInterface->getCache();

    // No tiles to fetch
    if (tileIndicesToFetch.empty()) {
        return;
    }

    // Fetch actual tile pointers on the cache, note that it may fail.
    std::vector<std::pair<int, void*> > fetchedTiles;
    void* cacheData;
    bool gotTiles = tileCache->getTilesPointer(_publicInterface->shared_from_this(), tileIndicesToFetch, &fetchedTiles, &cacheData);

    CacheDataLock_RAII cacheDataDeleter(tileCache, cacheData);
    if (!gotTiles) {
        throw std::bad_alloc();
    }

    // Recompose bits: associate tile coords to fetched pointers
    std::vector<FromCachePixelsTransferProcessor::CopyData> tilesToCopy(fetchedTiles.size());
    int k = 0;
    int channelTilesToFetchIndex = 0;
    for (std::size_t i = 0; i < fetchedTiles.size(); ++i, ++channelTilesToFetchIndex) {

        // Since all tiles are grouped, regroup to the appropriate channel
        if (channelTilesToFetchIndex >= (int)perChannelTilesToFetch[k].size()) {
            ++k;
            channelTilesToFetchIndex = 0;
        }

        FromCachePixelsTransferProcessor::CopyData &copyChunk = tilesToCopy[i];
        {
            copyChunk.tileData.bitDepth = bitdepth;
            copyChunk.tileData.nComps = 1;
            memset(copyChunk.tileData.ptrs, 0, sizeof(void*) * 4);
            copyChunk.tileData.ptrs[0] = fetchedTiles[i].second;
        }


        // Clamp the bounds of the tile so that it does not exceed the roi
        TileCoord& c = perChannelTilesToFetch[k][channelTilesToFetchIndex].coord;

        copyChunk.tileData.bounds.x1 = std::max(c.tx, roi.x1);
        copyChunk.tileData.bounds.y1 = std::max(c.ty, roi.y1);
        copyChunk.tileData.bounds.x2 = std::min(c.tx + tileSizeX, roi.x2);
        copyChunk.tileData.bounds.y2 = std::min(c.ty + tileSizeY, roi.y2);

        copyChunk.cpyArgs.roi = roi;
        copyChunk.cpyArgs.roi.intersect(copyChunk.tileData.bounds, &copyChunk.cpyArgs.roi);
        copyChunk.cpyArgs.monoConversion = Image::eMonoToPackedConversionCopyToChannelAndLeaveOthers;
        copyChunk.cpyArgs.conversionChannel = k;
    }

    // Finally copy over multiple threads each tile
    FromCachePixelsTransferProcessor processor(effect);
    processor.setValues(toData, tilesToCopy);
    ActionRetCodeEnum stat = processor.launchThreads();
    (void)stat;

} // fetchAndCopyCachedTiles

void
ImageCacheEntry::fromMemorySegment(ExternalSegmentType* segment,
                                   ExternalSegmentTypeHandleList::const_iterator start,
                                   ExternalSegmentTypeHandleList::const_iterator end)
{
    // Deserializes the tiles state
    {
        TileStateMap *stateCopy = (TileStateMap*)segment->get_address_from_handle(*start);
        assert(stateCopy->get_allocator().get_segment_manager() == segment->get_segment_manager());
        ++start;

        _imp->fetchAndCopyCachedTiles(*stateCopy);

    }

    CacheEntryBase::fromMemorySegment(segment, start, end);
    
    
} // fromMemorySegment


void
ImageCacheEntry::toMemorySegment(ExternalSegmentType* segment, ExternalSegmentTypeHandleList* objectPointers) const
{

    // Copy the tile state to the memory segment
    const TileStateMap& stateMap = _imp->state->getTilesMap();
    {

        void_allocator alloc(segment->get_segment_manager());

        TileStateMap *stateCopy = segment->construct<TileStateMap>(boost::interprocess::anonymous_instance)(alloc);
        if (!stateCopy) {
            throw std::bad_alloc();
        }
        *stateCopy = stateMap;
        objectPointers->push_back(segment->get_handle_from_address(stateCopy));
    }

    // Only RAM storage is supported when copying to tile storage.
    // We must have received a number of pointers that corresponds to the storage size.
    RAMImageStoragePtr storageIsRAM = toRAMImageStorage(_imp->storage);
    assert(storageIsRAM);
    assert(tileDataPtr.size() * NATRON_TILE_SIZE_BYTES <= storageIsRAM->getBufferSize());


    // The image data we want to copy from
    Image::CPUData fromData;
    {
        fromData.bitDepth = _imp->bitdepth;
        fromData.nComps = _imp->nComps;
        fromData.bounds = _imp->roi;
        memset(fromData.ptrs, 0, sizeof(void*) * 4);
        fromData.ptrs[0] = storageIsRAM->getData();
    }

    int tileSizeX, tileSizeY;
    _imp->state->getTileSize(&tileSizeX, &tileSizeY);

    // Build the pointers map to known to which tile corresponds which pointer
    typedef std::map<TileCoord, void*, TileCoord_Compare> TilePointersMap;
    TilePointersMap pointersMap;

    {
        TilePointersList::const_iterator it = tileDataPtr.begin();
        const RectI& pixelRoDRoundedToTileSize = _imp->state->getBoundsRoundedToTileSize();
        for (int ty = pixelRoDRoundedToTileSize.y1; ty < pixelRoDRoundedToTileSize.y2; ty += tileSizeY) {
            for (int tx = pixelRoDRoundedToTileSize.x1; tx < pixelRoDRoundedToTileSize.x2; tx += tileSizeX) {
                TileCoord c = {tx, ty};
                assert(it != tileDataPtr.end());
                pointersMap.insert(std::make_pair(c, *it));
            }
        }
    }

    std::vector<Image::CPUData> tilesToCopy;

    for (int ty = _imp->roiRoundedToTileSize.y1; ty < _imp->roiRoundedToTileSize.y2; ty += tileSizeY) {
        for (int tx = _imp->roiRoundedToTileSize.x1; tx < _imp->roiRoundedToTileSize.x2; tx += tileSizeX) {
            assert(tx % tileSizeX == 0 && ty % tileSizeY == 0);

            TileCoord c = {tx, ty};

            TilePointersMap::const_iterator foundTilePtr = pointersMap.find(c);
            assert(foundTilePtr != pointersMap.end());
            if (foundTilePtr == pointersMap.end()) {
                continue;
            }

            Image::CPUData toData;
            {
                toData.bitDepth = _imp->bitdepth;
                toData.nComps = 1;
                memset(toData.ptrs, 0, sizeof(void*) * 4);
                toData.ptrs[0] = foundTilePtr->second;
            }

            toData.bounds.x1 = std::max(c.tx, _imp->roi.x1);
            toData.bounds.y1 = std::max(c.ty, _imp->roi.y1);
            toData.bounds.x2 = std::min(c.tx + tileSizeX, _imp->roi.x2);
            toData.bounds.y2 = std::min(c.ty + tileSizeY, _imp->roi.y2);
            tilesToCopy.push_back(toData);
        }
    }
    ToCachePixelsTransferProcessor processor(_imp->effect);
    processor.setValues(fromData, tilesToCopy);
    ActionRetCodeEnum stat = processor.launchThreads();
    (void)stat;

    CacheEntryBase::toMemorySegment(segment, objectPointers);
} // toMemorySegment


NATRON_NAMESPACE_EXIT;
