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

#include "Image.h"

#include <algorithm> // min, max
#include <cassert>
#include <cstring> // for std::memcpy, std::memset
#include <stdexcept>

#include <QtCore/QDebug>

#include "Engine/ImagePrivate.h"


#ifndef M_LN2
#define M_LN2       0.693147180559945309417232121458176568  /* loge(2)        */
#endif

// When defined, tiles will be fetched from the cache (and optionnally downscaled) sequentially
//#define NATRON_IMAGE_SEQUENTIAL_INIT

NATRON_NAMESPACE_ENTER;

Image::Image()
: _imp(new ImagePrivate())
{

}

ImagePtr
Image::create(const InitStorageArgs& args)
{
    ImagePtr ret(new Image);
    ret->_imp->init(args);
    return ret;
}

Image::~Image()
{

    pushTilesToCacheIfNotAborted();
    
    // If this image is the last image holding a pointer to memory buffers, ensure these buffers
    // gets deallocated in a specific thread and not a render thread
    std::list<ImageStorageBasePtr> toDeleteInDeleterThread;
    for (TileMap::const_iterator it = _imp->tiles.begin(); it != _imp->tiles.end(); ++it) {
        for (std::size_t c = 0;  c < it->second.perChannelTile.size(); ++c) {
            toDeleteInDeleterThread.push_back(it->second.perChannelTile[c].buffer);
        }
    }

    _imp->tiles.clear();
    
    if (!toDeleteInDeleterThread.empty()) {
        appPTR->deleteCacheEntriesInSeparateThread(toDeleteInDeleterThread);
    }

}

void
Image::discardTiles()
{
    _imp->cachePolicy = eCacheAccessModeNone;
}

void
Image::pushTilesToCacheIfNotAborted()
{
   
    // Push tiles to cache if needed
    if (_imp->cachePolicy == eCacheAccessModeReadWrite ||
        _imp->cachePolicy == eCacheAccessModeWriteOnly) {
        assert(_imp->renderArgs);
        _imp->insertTilesInCache();
    }

}

bool
Image::waitForPendingTiles()
{
    if (_imp->cachePolicy == eCacheAccessModeNone) {
        return true;
    }
    bool hasStuffToRender = false;
    for (TileMap::const_iterator it = _imp->tiles.begin(); it != _imp->tiles.end(); ++it) {
        for (std::size_t c = 0; c < it->second.perChannelTile.size(); ++c) {
            if (it->second.perChannelTile[c].entryLocker) {
                if (it->second.perChannelTile[c].entryLocker->getStatus() == CacheEntryLocker::eCacheEntryStatusComputationPending) {
                    it->second.perChannelTile[c].entryLocker->waitForPendingEntry();
                }
                CacheEntryLocker::CacheEntryStatusEnum status = it->second.perChannelTile[c].entryLocker->getStatus();
                assert(status == CacheEntryLocker::eCacheEntryStatusCached || status == CacheEntryLocker::eCacheEntryStatusMustCompute);

                if (status == CacheEntryLocker::eCacheEntryStatusMustCompute) {
                    hasStuffToRender = true;
                }
            }
        }
    }
    return !hasStuffToRender;
} // waitForPendingTiles

Image::InitStorageArgs::InitStorageArgs()
: bounds()
, storage(eStorageModeRAM)
, bitdepth(eImageBitDepthFloat)
, layer(ImagePlaneDesc::getRGBAComponents())
, components()
, cachePolicy(eCacheAccessModeNone)
, bufferFormat(eImageBufferLayoutRGBAPackedFullRect)
, proxyScale(1.)
, mipMapLevel(0)
, isDraft(false)
, nodeTimeInvariantHash(0)
, time(0)
, view(0)
, glContext()
, textureTarget(GL_TEXTURE_2D)
, externalBuffer()
, delayAllocation(false)
{
    // By default make all channels
    components[0] = components[1] = components[2] = components[3] = 1;
}


class TileFetcherProcessor : public MultiThreadProcessorBase
{

    std::vector<TileCoord> _tileIndices;
    ImagePrivate* _imp;

public:

    TileFetcherProcessor(const TreeRenderNodeArgsPtr& renderArgs)
    : MultiThreadProcessorBase(renderArgs)
    {

    }

    virtual ~TileFetcherProcessor()
    {

    }

    void setData(ImagePrivate* imp)
    {
        // Initialize each tile
        for (int ty = imp->boundsRoundedToTile.y1; ty < imp->boundsRoundedToTile.y2; ty += imp->tileSizeY) {
            for (int tx = imp->boundsRoundedToTile.x1; tx < imp->boundsRoundedToTile.x2; tx += imp->tileSizeX) {
                TileCoord c = {tx, ty};
                _tileIndices.push_back(c);
            }
        }
        _imp = imp;
    }

    virtual ActionRetCodeEnum launchThreads(unsigned int nCPUs = 0) OVERRIDE FINAL WARN_UNUSED_RETURN
    {
        return MultiThreadProcessorBase::launchThreads(nCPUs);
    }

    virtual ActionRetCodeEnum multiThreadFunction(unsigned int threadID,
                                                  unsigned int nThreads,
                                                  const TreeRenderNodeArgsPtr& /*renderArgs*/) OVERRIDE FINAL WARN_UNUSED_RETURN
    {
        // Each threads get a rectangular portion but full scan-lines
        int fromIndex, toIndex;
        ImageMultiThreadProcessorBase::getThreadRange(threadID, nThreads, 0, _tileIndices.size(), &fromIndex, &toIndex);
        for (int i = fromIndex; i < toIndex; ++i) {
            const TileCoord& c = _tileIndices[i];
            try {
                _imp->initTileAndFetchFromCache(c.tx, c.ty);
            } catch (...) {
                return eActionStatusFailed;
            }
        }
        return eActionStatusOK;
    }
};

void
ImagePrivate::init(const Image::InitStorageArgs& args)
{

    // Should be initialized once!
    assert(tiles.empty());
    if (!tiles.empty()) {
        throw std::bad_alloc();
    }

    // The bounds of the image must not be empty
    if (args.bounds.isNull()) {
        throw std::bad_alloc();
    }

    RenderScale proxyPlusMipMapScale = args.proxyScale;
    {
        double mipMapScale = Image::getScaleFromMipMapLevel(args.mipMapLevel);
        proxyPlusMipMapScale.x *= mipMapScale;
        proxyPlusMipMapScale.y *= mipMapScale;
    }

    originalBounds = args.bounds;
    boundsRoundedToTile = originalBounds;
    cachePolicy = args.cachePolicy;
    if (args.storage != eStorageModeDisk) {
        // We can only cache stuff on disk.
        cachePolicy = eCacheAccessModeNone;
    }
    bufferFormat = args.bufferFormat;
    layer = args.layer;
    proxyScale = args.proxyScale;
    mipMapLevel = args.mipMapLevel;
    renderArgs = args.renderArgs;
    enabledChannels = args.components;
    bitdepth = args.bitdepth;
    tilesAllocated = args.delayAllocation;
    storage = args.storage;
    glContext = args.glContext;
    textureTarget = args.textureTarget;
    nodeHash = args.nodeTimeInvariantHash;
    time = args.time;
    view = args.view;
    isDraftImage = args.isDraft;

    // OpenGL texture back-end only supports 32-bit float RGBA packed format.
    assert(args.storage != eStorageModeGLTex || (args.bufferFormat == eImageBufferLayoutRGBAPackedFullRect && args.bitdepth == eImageBitDepthFloat));
    if (args.storage == eStorageModeGLTex && (args.bufferFormat != eImageBufferLayoutRGBAPackedFullRect || args.bitdepth != eImageBitDepthFloat)) {
        throw std::bad_alloc();
    }

    // MMAP storage only supports mono channel tiles, except if it is an external buffer.
    assert(args.storage != eStorageModeDisk || args.bufferFormat == eImageBufferLayoutMonoChannelTiled || args.externalBuffer);
    if (args.storage == eStorageModeDisk && args.bufferFormat != eImageBufferLayoutMonoChannelTiled && !args.externalBuffer) {
        throw std::bad_alloc();
    }

    // If allocating OpenGL textures, ensure the context is current
    OSGLContextAttacherPtr contextLocker;
    if (args.storage == eStorageModeGLTex) {
        contextLocker = OSGLContextAttacher::create(args.glContext);
        contextLocker->attach();
    }


    // For tiled layout, get the number of tiles in X and Y depending on the bounds and the tile zie.
    switch (args.bufferFormat) {
        case eImageBufferLayoutMonoChannelTiled: {
            // The size of a tile depends on the bitdepth
            Cache::getTileSizePx(args.bitdepth, &tileSizeX, &tileSizeY);
            boundsRoundedToTile.roundToTileSize(tileSizeX, tileSizeY);
        }   break;
        case eImageBufferLayoutRGBACoplanarFullRect:
        case eImageBufferLayoutRGBAPackedFullRect:
            tileSizeX = originalBounds.width();
            tileSizeY = originalBounds.height();
            break;
    }


    if (args.externalBuffer) {
        initFromExternalBuffer(args);
    } else {
        initTiles();
    }
} // init

void
ImagePrivate::initTiles()
{
    if (tileSizeX == boundsRoundedToTile.width() && tileSizeY == boundsRoundedToTile.height()) {
        // Single tile
        initTileAndFetchFromCache(0, 0);
    } else {
#ifdef NATRON_IMAGE_SEQUENTIAL_INIT
        for (int ty = imp->boundsRoundedToTile.y1; ty < imp->boundsRoundedToTile.y2; ty += tileSizeY) {
            for (int tx = imp->boundsRoundedToTile.x1; tx < imp->boundsRoundedToTile.x2; tx += tileSizeX) {
                initTileAndFetchFromCache(tx, ty);
            }
        }
#else
        TileFetcherProcessor processor(renderArgs);
        processor.setData(this);
        ActionRetCodeEnum stat = processor.launchThreads();
        if (stat == eActionStatusFailed) {
            throw std::bad_alloc();
        }
#endif
#ifdef DEBUG
        for (TileMap::const_iterator it = tiles.begin(); it != tiles.end(); ++it) {
            assert(it->second.perChannelTile.size() > 0);
        }
#endif
    }

}

Image::CopyPixelsArgs::CopyPixelsArgs()
: roi()
, conversionChannel(0)
, alphaHandling(Image::eAlphaChannelHandlingFillFromChannel)
, monoConversion(Image::eMonoToPackedConversionCopyToChannelAndLeaveOthers)
, srcColorspace(eViewerColorSpaceLinear)
, dstColorspace(eViewerColorSpaceLinear)
, unPremultIfNeeded(false)
, skipDestinationTilesMarkedCached(false)
, forceCopyEvenIfBuffersHaveSameLayout(false)
{

}


void
Image::copyPixels(const Image& other, const CopyPixelsArgs& args)
{
    if (_imp->tiles.empty() || other._imp->tiles.empty()) {
        // Nothing to copy
        return;
    }

    // Roi must intersect both images bounds
    RectI roi;
    if (!other._imp->originalBounds.intersect(args.roi, &roi)) {
        return;
    }
    if (!_imp->originalBounds.intersect(args.roi, &roi)) {
        return;
    }


    // Optimize: try to just copy the memory buffer pointers instead of copying the memory itself
    if (!args.forceCopyEvenIfBuffersHaveSameLayout && roi == _imp->originalBounds) {

        bool copyPointers = true;
        if (_imp->tiles.size() != other._imp->tiles.size()) {
            copyPointers = false;
        }
        if (copyPointers) {
            StorageModeEnum thisStorage = getStorageMode();
            StorageModeEnum otherStorage = other.getStorageMode();
            if ((thisStorage == eStorageModeGLTex || otherStorage == eStorageModeGLTex)) {
                copyPointers = false;
            }
        }
        if (copyPointers && _imp->originalBounds != other._imp->originalBounds) {
            copyPointers = false;
        }
        if (copyPointers && getBitDepth() != other.getBitDepth()) {
            copyPointers = false;
        }
        if (copyPointers && _imp->tiles.begin()->second.perChannelTile.size() != other._imp->tiles.begin()->second.perChannelTile.size()) {
            copyPointers = false;
        }
        if (copyPointers && _imp->layer.getNumComponents() != other._imp->layer.getNumComponents()) {
            copyPointers = false;
        }
        // Only support copying buffers with different layouts if they have 1 component only
        if (copyPointers) {
            ImageBufferLayoutEnum thisLayout = _imp->bufferFormat;
            ImageBufferLayoutEnum otherFormat = other._imp->bufferFormat;
            if (thisLayout != otherFormat && _imp->layer.getNumComponents() != 1) {
                copyPointers = false;
            }
        }

        if (copyPointers) {
            assert(_imp->tiles.size() == other._imp->tiles.size());
            TileMap::iterator oit = other._imp->tiles.begin();
            for (TileMap::iterator it = _imp->tiles.begin(); it != _imp->tiles.end(); ++it, ++oit) {
                assert(it->second.perChannelTile.size() == oit->second.perChannelTile.size());
                for (std::size_t c = 0; c < it->second.perChannelTile.size(); ++c) {
                    it->second.perChannelTile[c].buffer = oit->second.perChannelTile[c].buffer;
                }
            }
        }

    } // !args.forceCopyEvenIfBuffersHaveSameLayout

    ImagePtr tmpImage = ImagePrivate::checkIfCopyToTempImageIsNeeded(other, *this, roi);

    const Image* fromImage = tmpImage? tmpImage.get() : &other;

    // Update the roi before calling copyRectangle
    CopyPixelsArgs argsCpy = args;
    argsCpy.roi = roi;

    if (_imp->bufferFormat == eImageBufferLayoutMonoChannelTiled) {
        // UNTILED ---> TILED
        _imp->copyUntiledImageToTiledImage(*fromImage, argsCpy);

    } else {

        // Optimize the case where nobody is tiled
        if (fromImage->_imp->bufferFormat != eImageBufferLayoutMonoChannelTiled) {

            // UNTILED ---> UNTILED
            _imp->copyUntiledImageToUntiledImage(*fromImage, argsCpy);

        } else {
            // TILED ---> UNTILED
            _imp->copyTiledImageToUntiledImage(*fromImage, argsCpy);
        }

    } // isTiled
} // copyPixels

void
Image::ensureBuffersAllocated()
{
    QMutexLocker k(&_imp->tilesAllocatedMutex);
    for (TileMap::iterator it = _imp->tiles.begin(); it != _imp->tiles.end(); ++it) {
        for (std::size_t c = 0; c < it->second.perChannelTile.size(); ++c) {
            if (it->second.perChannelTile[c].buffer->hasAllocateMemoryArgs()) {
                it->second.perChannelTile[c].buffer->allocateMemoryFromSetArgs();
            }
        }
    }
    _imp->tilesAllocated = true;
} // ensureBuffersAllocated

ImageBufferLayoutEnum
Image::getBufferFormat() const
{
    return _imp->bufferFormat;
}

StorageModeEnum
Image::getStorageMode() const
{
    if (_imp->tiles.empty()) {
        return eStorageModeNone;
    }
    assert(!_imp->tiles.begin()->second.perChannelTile.empty() && _imp->tiles.begin()->second.perChannelTile[0].buffer);
    return _imp->tiles.begin()->second.perChannelTile[0].buffer->getStorageMode();
}

const RectI&
Image::getBounds() const
{
    return _imp->originalBounds;
}

const RenderScale&
Image::getProxyScale() const
{
    return _imp->proxyScale;
}

unsigned int
Image::getMipMapLevel() const
{
    return _imp->mipMapLevel;
}


double
Image::getScaleFromMipMapLevel(unsigned int level)
{
    return 1. / (1 << level);
}


unsigned int
Image::getLevelFromScale(double s)
{
    assert(0. < s && s <= 1.);
    int retval = -std::floor(std::log(s) / M_LN2 + 0.5);
    assert(retval >= 0);
    return retval;
}

unsigned int
Image::getComponentsCount() const
{
    return _imp->layer.getNumComponents();
}


const ImagePlaneDesc&
Image::getLayer() const
{
    return _imp->layer;
}


ImageBitDepthEnum
Image::getBitDepth() const
{
    return _imp->bitdepth;
}

GLImageStoragePtr
Image::getGLImageStorage() const
{
    if (_imp->tiles.empty()) {
        return GLImageStoragePtr();
    }
    if (_imp->tiles.begin()->second.perChannelTile.empty()) {
        return GLImageStoragePtr();
    }
    GLImageStoragePtr isGLEntry = toGLImageStorage(_imp->tiles.begin()->second.perChannelTile[0].buffer);
    return isGLEntry;
}

void
Image::getCPUTileData(const Tile& tile, ImageBufferLayoutEnum layout, CPUTileData* data)
{
    memset(data->ptrs, 0, sizeof(void*) * 4);
    data->nComps = 0;
    data->bitDepth = eImageBitDepthNone;

    for (std::size_t i = 0; i < tile.perChannelTile.size(); ++i) {
        RAMImageStoragePtr fromIsRAMBuffer = toRAMImageStorage(tile.perChannelTile[i].buffer);
        CacheImageTileStoragePtr fromIsMMAPBuffer = toCacheImageTileStorage(tile.perChannelTile[i].buffer);

        if (!fromIsMMAPBuffer && !fromIsRAMBuffer) {
            continue;
        }
        if (i == 0) {
            if (fromIsRAMBuffer) {
                data->ptrs[0] = fromIsRAMBuffer->getData();
                data->tileBounds = fromIsRAMBuffer->getBounds();
                data->bitDepth = fromIsRAMBuffer->getBitDepth();
                data->nComps = fromIsRAMBuffer->getNumComponents();

                if (layout == eImageBufferLayoutRGBACoplanarFullRect) {
                    // Coplanar requires offsetting
                    assert(tile.perChannelTile.size() == 1);
                    std::size_t planeSize = data->nComps * data->tileBounds.area() * getSizeOfForBitDepth(data->bitDepth);
                    if (data->nComps > 1) {
                        data->ptrs[1] = (char*)data->ptrs[0] + planeSize;
                        if (data->nComps > 2) {
                            data->ptrs[2] = (char*)data->ptrs[1] + planeSize;
                            if (data->nComps > 3) {
                                data->ptrs[3] = (char*)data->ptrs[2] + planeSize;
                            }
                        }
                    }
                }
            } else {
                data->ptrs[0] = fromIsMMAPBuffer->getData();
                data->tileBounds = fromIsMMAPBuffer->getBounds();
                data->bitDepth = fromIsMMAPBuffer->getBitDepth();
                data->nComps = tile.perChannelTile.size();
            }
        } else {
            assert(layout == eImageBufferLayoutMonoChannelTiled);
            int channelIndex = tile.perChannelTile[i].channelIndex;
            assert(channelIndex >= 0 && channelIndex < 4);
            if (fromIsRAMBuffer) {
                data->ptrs[channelIndex] = fromIsRAMBuffer->getData();
            } else {
                data->ptrs[channelIndex] = fromIsMMAPBuffer->getData();
            }
        }
    } // for each channel
} // getCPUTileData

void
Image::getCPUTileData(const Image::Tile& tile,
                      Image::CPUTileData* data) const
{
    getCPUTileData(tile, _imp->bufferFormat, data);
}

void
Image::getTileSize(int *tileSizeX, int* tileSizeY) const
{
    *tileSizeX = _imp->tileSizeX;
    *tileSizeY = _imp->tileSizeY;
}

int
Image::getNumTiles() const
{
    return (int)_imp->tiles.size();
}


CacheAccessModeEnum
Image::getCachePolicy() const
{
    return _imp->cachePolicy;
}

void
Image::getRestToRender(TileStateMap* tileStatus, bool* hasUnRenderedTile, bool *hasPendingResults) const
{


    for (TileMap::iterator it = _imp->tiles.begin(); it != _imp->tiles.end(); ++it) {

        TileState &state = (*tileStatus)[it->first];
        bool hasChannelNotCached = false;
        bool hasChannelPending = false;
        for (std::size_t c = 0; c < it->second.perChannelTile.size(); ++c) {

            CacheEntryLocker::CacheEntryStatusEnum status;
            if (it->second.perChannelTile[c].entryLocker) {
                status = it->second.perChannelTile[c].entryLocker->getStatus();
            } else {
                status = CacheEntryLocker::eCacheEntryStatusMustCompute;
            }

            if (status == CacheEntryLocker::eCacheEntryStatusMustCompute) {
                hasChannelNotCached = true;
            }
            if (status == CacheEntryLocker::eCacheEntryStatusComputationPending) {
                hasChannelPending = true;
            }
        }
        state.status = eTileStatusNotRendered;
         if (!hasChannelNotCached && hasChannelPending) {
            state.status = eTileStatusPending;
        } else if (!hasChannelPending && !hasChannelNotCached) {
            state.status = eTileStatusRendered;
        }
        if (state.status == eTileStatusPending) {
            *hasPendingResults = true;
        } else if (state.status == eTileStatusNotRendered) {
            *hasUnRenderedTile = true;
        }
        state.bounds = it->second.tileBounds;
    }
} // getRestToRender

bool
Image::getTileAt(int tx, int ty, Image::Tile* tile) const
{
    assert(tile);
    TileCoord coord = {tx, ty};
    TileMap::const_iterator found = _imp->tiles.find(coord);
    if (found == _imp->tiles.end()) {
        return false;
    }
    *tile = found->second;
    return true;
}

void
Image::getABCDRectangles(const RectI& srcBounds, const RectI& biggerBounds, RectI& aRect, RectI& bRect, RectI& cRect, RectI& dRect)
{
    /*
     Compute the rectangles (A,B,C,D) where to set the image to 0

     AAAAAAAAAAAAAAAAAAAAAAAAAAAA
     AAAAAAAAAAAAAAAAAAAAAAAAAAAA
     DDDDDXXXXXXXXXXXXXXXXXXBBBBB
     DDDDDXXXXXXXXXXXXXXXXXXBBBBB
     DDDDDXXXXXXXXXXXXXXXXXXBBBBB
     DDDDDXXXXXXXXXXXXXXXXXXBBBBB
     CCCCCCCCCCCCCCCCCCCCCCCCCCCC
     CCCCCCCCCCCCCCCCCCCCCCCCCCCC
     */
    aRect.x1 = biggerBounds.x1;
    aRect.y1 = srcBounds.y2;
    aRect.y2 = biggerBounds.y2;
    aRect.x2 = biggerBounds.x2;

    bRect.x1 = srcBounds.x2;
    bRect.y1 = srcBounds.y1;
    bRect.x2 = biggerBounds.x2;
    bRect.y2 = srcBounds.y2;

    cRect.x1 = biggerBounds.x1;
    cRect.y1 = biggerBounds.y1;
    cRect.x2 = biggerBounds.x2;
    cRect.y2 = srcBounds.y1;

    dRect.x1 = biggerBounds.x1;
    dRect.y1 = srcBounds.y1;
    dRect.x2 = srcBounds.x1;
    dRect.y2 = srcBounds.y2;

} // getABCDRectangles

static void getImageBoundsFromTilesState(const Image::TileStateMap& tiles, int tileSizeX, int tileSizeY,
                                         RectI& imageBoundsRoundedToTileSize,
                                         RectI& imageBoundsNotRounded)
{
    {
        Image::TileStateMap::const_iterator bottomLeft = tiles.begin();
        Image::TileStateMap::const_reverse_iterator topRight = tiles.rbegin();
        imageBoundsRoundedToTileSize.x1 = bottomLeft->first.tx;
        imageBoundsRoundedToTileSize.y1 = bottomLeft->first.ty;
        imageBoundsRoundedToTileSize.x2 = topRight->first.tx + tileSizeX;
        imageBoundsRoundedToTileSize.y2 = topRight->first.ty + tileSizeY;

        imageBoundsNotRounded.x1 = bottomLeft->second.bounds.x1;
        imageBoundsNotRounded.y1 = bottomLeft->second.bounds.y1;
        imageBoundsNotRounded.x2 = topRight->second.bounds.x2;
        imageBoundsNotRounded.y2 = topRight->second.bounds.y2;
    }

    assert(imageBoundsRoundedToTileSize.x1 % tileSizeX == 0);
    assert(imageBoundsRoundedToTileSize.y1 % tileSizeY == 0);
    assert(imageBoundsRoundedToTileSize.x2 % tileSizeX == 0);
    assert(imageBoundsRoundedToTileSize.y2 % tileSizeY == 0);
}

RectI
Image::getMinimalBboxToRenderFromTilesState(const TileStateMap& tiles, const RectI& roi, int tileSizeX, int tileSizeY)
{

    if (tiles.empty()) {
        return RectI();
    }

    RectI imageBoundsRoundedToTileSize;
    RectI imageBoundsNotRounded;
    getImageBoundsFromTilesState(tiles, tileSizeX, tileSizeY, imageBoundsRoundedToTileSize, imageBoundsNotRounded);
    assert(imageBoundsRoundedToTileSize.contains(roi));

    RectI roiRoundedToTileSize = roi;
    roiRoundedToTileSize.roundToTileSize(tileSizeX, tileSizeY);

    // Search for rendered lines from bottom to top
    for (int y = roiRoundedToTileSize.y1; y < roiRoundedToTileSize.y2; y += tileSizeY) {

        bool hasTileUnrenderedOnLine = false;
        for (int x = roiRoundedToTileSize.x1; x < roiRoundedToTileSize.x2; x += tileSizeX) {

            TileCoord c = {x, y};
            TileStateMap::const_iterator foundTile = tiles.find(c);
            assert(foundTile != tiles.end());
            if (foundTile->second.status == eTileStatusNotRendered) {
                hasTileUnrenderedOnLine = true;
                break;
            }
        }
        if (!hasTileUnrenderedOnLine) {
            roiRoundedToTileSize.y1 += tileSizeY;
        }
    }

    // Search for rendered lines from top to bottom
    for (int y = roiRoundedToTileSize.y2 - 1; y >= roiRoundedToTileSize.y1; y -= tileSizeY) {

        bool hasTileUnrenderedOnLine = false;
        for (int x = roiRoundedToTileSize.x1; x < roiRoundedToTileSize.x2; x += tileSizeX) {

            TileCoord c = {x, y};
            TileStateMap::const_iterator foundTile = tiles.find(c);
            assert(foundTile != tiles.end());
            if (foundTile->second.status == eTileStatusNotRendered) {
                hasTileUnrenderedOnLine = true;
                break;
            }
        }
        if (!hasTileUnrenderedOnLine) {
            roiRoundedToTileSize.y2 -= tileSizeY;
        }
    }

    // Avoid making roiRoundedToTileSize.width() iterations for nothing
    if (roiRoundedToTileSize.isNull()) {
        return roiRoundedToTileSize;
    }


    // Search for rendered columns from left to right
    for (int x = roiRoundedToTileSize.x1; x < roiRoundedToTileSize.x2; x += tileSizeX) {

        bool hasTileUnrenderedOnCol = false;
        for (int y = roiRoundedToTileSize.y1; y <= roiRoundedToTileSize.y2; y += tileSizeY) {

            TileCoord c = {x, y};
            TileStateMap::const_iterator foundTile = tiles.find(c);
            assert(foundTile != tiles.end());
            if (foundTile->second.status == eTileStatusNotRendered) {
                hasTileUnrenderedOnCol = true;
                break;
            }
        }
        if (!hasTileUnrenderedOnCol) {
            roiRoundedToTileSize.x1 += tileSizeX;
        }
    }

    // Avoid making roiRoundedToTileSize.width() iterations for nothing
    if (roiRoundedToTileSize.isNull()) {
        return roiRoundedToTileSize;
    }

    // Search for rendered columns from right to left
    for (int x = roiRoundedToTileSize.x2 - 1; x >= roiRoundedToTileSize.x1; x -= tileSizeX) {

        bool hasTileUnrenderedOnCol = false;
        for (int y = roiRoundedToTileSize.y1; y <= roiRoundedToTileSize.y2; y += tileSizeY) {

            TileCoord c = {x, y};
            TileStateMap::const_iterator foundTile = tiles.find(c);
            assert(foundTile != tiles.end());
            if (foundTile->second.status == eTileStatusNotRendered) {
                hasTileUnrenderedOnCol = true;
                break;
            }
        }
        if (!hasTileUnrenderedOnCol) {
            roiRoundedToTileSize.x2 -= tileSizeX;
        }
    }

    // Intersect the result to the actual image bounds (because the tiles are rounded to tile size)
    RectI ret;
    roiRoundedToTileSize.intersect(imageBoundsNotRounded, &ret);
    return ret;

    
} // getMinimalBboxToRenderFromTilesState

void
Image::getMinimalRectsToRenderFromTilesState(const TileStateMap& tiles, const RectI& roi, int tileSizeX, int tileSizeY, std::list<RectI>* rectsToRender)
{
    if (tiles.empty()) {
        return;
    }

    RectI roiRoundedToTileSize = roi;
    roiRoundedToTileSize.roundToTileSize(tileSizeX, tileSizeY);

    RectI bboxM = getMinimalBboxToRenderFromTilesState(tiles, roi, tileSizeX, tileSizeY);
    if (bboxM.isNull()) {
        return;
    }

    // optimization by Fred, Jan 31, 2014
    //
    // Now that we have the smallest enclosing bounding box,
    // let's try to find rectangles for the bottom, the top,
    // the left and the right part.
    // This happens quite often, for example when zooming out
    // (in this case the area to compute is formed of A, B, C and D,
    // and X is already rendered), or when panning (in this case the area
    // is just two rectangles, e.g. A and C, and the rectangles B, D and
    // X are already rendered).
    // The rectangles A, B, C and D from the following drawing are just
    // zeroes, and X contains zeroes and ones.
    //
    // BBBBBBBBBBBBBB
    // BBBBBBBBBBBBBB
    // CXXXXXXXXXXDDD
    // CXXXXXXXXXXDDD
    // CXXXXXXXXXXDDD
    // CXXXXXXXXXXDDD
    // AAAAAAAAAAAAAA

    // First, find if there's an "A" rectangle, and push it to the result
    //find bottom
    RectI bboxX = bboxM;
    RectI bboxA = bboxX;
    bboxA.y2 = bboxX.y1;
    for (int y = bboxX.y1; y < bboxX.y2; y += tileSizeY) {
        bool hasRenderedTileOnLine = false;
        for (int x = bboxX.x1; x < bboxX.x2; x += tileSizeX) {
            TileCoord c = {x, y};
            TileStateMap::const_iterator foundTile = tiles.find(c);
            assert(foundTile != tiles.end());
            if (foundTile->second.status != eTileStatusNotRendered) {
                hasRenderedTileOnLine = true;
                break;
            }
        }
        if (hasRenderedTileOnLine) {
            break;
        } else {
            ++bboxX.y1;
            bboxA.y2 = bboxX.y1;
        }
    }
    if ( !bboxA.isNull() ) { // empty boxes should not be pushed
        rectsToRender->push_back(bboxA);
    }

    // Now, find the "B" rectangle
    //find top
    RectI bboxB = bboxX;
    bboxB.y1 = bboxX.y2;

    for (int y = bboxX.y2 - 1; y >= bboxX.y1; y -= tileSizeY) {
        bool hasRenderedTileOnLine = false;
        for (int x = bboxX.x1; x < bboxX.x2; x += tileSizeX) {
            TileCoord c = {x, y};
            TileStateMap::const_iterator foundTile = tiles.find(c);
            assert(foundTile != tiles.end());
            if (foundTile->second.status != eTileStatusNotRendered) {
                hasRenderedTileOnLine = true;
                break;
            }
        }
        if (hasRenderedTileOnLine) {
            break;
        } else {
            --bboxX.y2;
            bboxB.y1 = bboxX.y2;
        }
    }

    if ( !bboxB.isNull() ) { // empty boxes should not be pushed
        rectsToRender->push_back(bboxB);
    }

    //find left
    RectI bboxC = bboxX;
    bboxC.x2 = bboxX.x1;
    if ( bboxX.y1 < bboxX.y2 ) {
        for (int x = bboxX.x1; x < bboxX.x2; x += tileSizeX) {
            bool hasRenderedTileOnCol = false;
            for (int y = bboxX.y1; y < bboxX.y2; y += tileSizeY) {
                TileCoord c = {x, y};
                TileStateMap::const_iterator foundTile = tiles.find(c);
                assert(foundTile != tiles.end());
                if (foundTile->second.status != eTileStatusNotRendered) {
                    hasRenderedTileOnCol = true;
                    break;
                }

            }
            if (hasRenderedTileOnCol) {
                break;
            } else {
                ++bboxX.x1;
                bboxC.x2 = bboxX.x1;
            }
        }
    }
    if ( !bboxC.isNull() ) { // empty boxes should not be pushed
        rectsToRender->push_back(bboxC);
    }

    //find right
    RectI bboxD = bboxX;
    bboxD.x1 = bboxX.x2;
    if ( bboxX.y1 < bboxX.y2 ) {
        for (int x = bboxX.x2 - 1; x >= bboxX.x1; x -= tileSizeX) {
            bool hasRenderedTileOnCol = false;
            for (int y = bboxX.y1; y < bboxX.y2; y += tileSizeY) {
                TileCoord c = {x, y};
                TileStateMap::const_iterator foundTile = tiles.find(c);
                assert(foundTile != tiles.end());
                if (foundTile->second.status != eTileStatusNotRendered) {
                    hasRenderedTileOnCol = true;
                    break;
                }
            }
            if (hasRenderedTileOnCol) {
                break;
            } else {
                --bboxX.x2;
                bboxD.x1 = bboxX.x2;
            }
        }
    }
    if ( !bboxD.isNull() ) { // empty boxes should not be pushed
        rectsToRender->push_back(bboxD);
    }

    assert( bboxA.bottom() == bboxM.bottom() );
    assert( bboxA.left() == bboxM.left() );
    assert( bboxA.right() == bboxM.right() );
    assert( bboxA.top() == bboxX.bottom() );

    assert( bboxB.top() == bboxM.top() );
    assert( bboxB.left() == bboxM.left() );
    assert( bboxB.right() == bboxM.right() );
    assert( bboxB.bottom() == bboxX.top() );

    assert( bboxC.top() == bboxX.top() );
    assert( bboxC.left() == bboxM.left() );
    assert( bboxC.right() == bboxX.left() );
    assert( bboxC.bottom() == bboxX.bottom() );

    assert( bboxD.top() == bboxX.top() );
    assert( bboxD.left() == bboxX.right() );
    assert( bboxD.right() == bboxM.right() );
    assert( bboxD.bottom() == bboxX.bottom() );

    // get the bounding box of what's left (the X rectangle in the drawing above)
    bboxX = getMinimalBboxToRenderFromTilesState(tiles, bboxX, tileSizeX, tileSizeY);

    if ( !bboxX.isNull() ) { // empty boxes should not be pushed
        rectsToRender->push_back(bboxX);
    }

} // getMinimalRectsToRenderFromTilesState

class FillProcessor : public ImageMultiThreadProcessorBase
{
    void* _ptrs[4];
    RectI _tileBounds;
    ImageBitDepthEnum _bitDepth;
    int _nComps;
    RGBAColourF _color;

public:

    FillProcessor(const TreeRenderNodeArgsPtr& renderArgs)
    : ImageMultiThreadProcessorBase(renderArgs)
    {

    }

    virtual ~FillProcessor()
    {
    }

    void setValues(void* ptrs[4], const RectI& tileBounds, ImageBitDepthEnum bitDepth, int nComps, const RGBAColourF& color)
    {
        memcpy(_ptrs, ptrs, sizeof(void*) * 4);
        _tileBounds = tileBounds;
        _bitDepth = bitDepth;
        _nComps = nComps;
        _color = color;
    }

private:

    virtual ActionRetCodeEnum multiThreadProcessImages(const RectI& renderWindow, const TreeRenderNodeArgsPtr& renderArgs) OVERRIDE FINAL
    {
        ImagePrivate::fillCPU(_ptrs, _color.r, _color.g, _color.b, _color.a, _nComps, _bitDepth, _tileBounds, renderWindow, renderArgs);
        return eActionStatusOK;
    }
};

void
Image::fill(const RectI & roi,
            float r,
            float g,
            float b,
            float a)
{
    if (_imp->tiles.empty()) {
        return;
    }

    if (getStorageMode() == eStorageModeGLTex) {
        GLImageStoragePtr glEntry = toGLImageStorage(_imp->tiles.begin()->second.perChannelTile[0].buffer);
        _imp->fillGL(roi, r, g, b, a, glEntry);
        return;
    }



    for (TileMap::iterator it = _imp->tiles.begin(); it != _imp->tiles.end(); ++it) {

        Image::CPUTileData tileData;
        getCPUTileData(it->second, &tileData);
        RectI tileRoI;
        roi.intersect(tileData.tileBounds, &tileRoI);

        RGBAColourF color = {r, g, b, a};

        FillProcessor processor(_imp->renderArgs);
        processor.setValues(tileData.ptrs, tileData.tileBounds, tileData.bitDepth, tileData.nComps, color);
        processor.setRenderWindow(tileRoI);
        processor.process();

    }

} // fill

void
Image::fillZero(const RectI& roi)
{
    fill(roi, 0., 0., 0., 0.);
}

void
Image::fillBoundsZero()
{
    fillZero(getBounds());
}

void
Image::ensureBounds(const RectI& roi)
{
    if (_imp->originalBounds.contains(roi)) {
        return;
    }

    RectI oldBounds = _imp->originalBounds;
    RectI newBounds = oldBounds;
    newBounds.merge(roi);

    if (_imp->bufferFormat == eImageBufferLayoutMonoChannelTiled) {

        _imp->originalBounds = newBounds;
        _imp->boundsRoundedToTile = newBounds;
        _imp->boundsRoundedToTile.roundToTileSize(_imp->tileSizeX, _imp->tileSizeY);
        _imp->initTiles();
    } else {
        ImagePtr tmpImage;
        {
            Image::InitStorageArgs initArgs;
            initArgs.bounds = newBounds;
            initArgs.layer = getLayer();
            initArgs.bitdepth = getBitDepth();
            initArgs.bufferFormat = getBufferFormat();
            initArgs.storage = getStorageMode();
            initArgs.mipMapLevel = getMipMapLevel();
            initArgs.proxyScale = getProxyScale();
            GLImageStoragePtr isGlEntry = getGLImageStorage();
            if (isGlEntry) {
                initArgs.textureTarget = isGlEntry->getGLTextureTarget();
                initArgs.glContext = isGlEntry->getOpenGLContext();
            }
            tmpImage = Image::create(initArgs);
        }
        Image::CopyPixelsArgs cpyArgs;
        cpyArgs.roi = oldBounds;
        tmpImage->copyPixels(*this, cpyArgs);

        // Swap images so that this image becomes the resized one.
        _imp.swap(tmpImage->_imp);

    }
} // ensureBounds

void
Image::getChannelPointers(const void* ptrs[4],
                          int x, int y,
                          const RectI& bounds,
                          int nComps,
                          ImageBitDepthEnum bitdepth,
                          void* outPtrs[4],
                          int* pixelStride)
{
    switch (bitdepth) {
        case eImageBitDepthByte:
            getChannelPointers<unsigned char>((const unsigned char**)ptrs, x, y, bounds, nComps, (unsigned char**)outPtrs, pixelStride);
            break;
        case eImageBitDepthShort:
            getChannelPointers<unsigned short>((const unsigned short**)ptrs, x, y, bounds, nComps, (unsigned short**)outPtrs, pixelStride);
            break;
        case eImageBitDepthFloat:
            getChannelPointers<float>((const float**)ptrs, x, y, bounds, nComps, (float**)outPtrs, pixelStride);
            break;
        default:
            memset(outPtrs, 0, sizeof(void*) * 4);
            *pixelStride = 0;
            break;
    }
}


const unsigned char*
Image::pixelAtStatic(int x,
                     int y,
                     const RectI& bounds,
                     int nComps,
                     int dataSizeOf,
                     const unsigned char* buf)
{
    if ( ( x < bounds.x1 ) || ( x >= bounds.x2 ) || ( y < bounds.y1 ) || ( y >= bounds.y2 ) ) {
        return NULL;
    } else {
        const unsigned char* ret = buf;
        if (!ret) {
            return 0;
        }
        std::size_t compDataSize = dataSizeOf * nComps;
        ret = ret + (std::size_t)( y - bounds.y1 ) * compDataSize * bounds.width()
        + (std::size_t)( x - bounds.x1 ) * compDataSize;

        return ret;
    }
}

unsigned char*
Image::pixelAtStatic(int x,
                     int y,
                     const RectI& bounds,
                     int nComps,
                     int dataSizeOf,
                     unsigned char* buf)
{
    if ( ( x < bounds.x1 ) || ( x >= bounds.x2 ) || ( y < bounds.y1 ) || ( y >= bounds.y2 ) ) {
        return NULL;
    } else {
        unsigned char* ret = buf;
        if (!ret) {
            return 0;
        }
        std::size_t compDataSize = dataSizeOf * nComps;
        ret = ret + (std::size_t)( y - bounds.y1 ) * compDataSize * bounds.width()
              + (std::size_t)( x - bounds.x1 ) * compDataSize;

        return ret;
    }
}

std::string
Image::getFormatString(const ImagePlaneDesc& comps,
                       ImageBitDepthEnum depth)
{
    std::string s = comps.getPlaneLabel() + '.' + comps.getChannelsLabel();

    s.append( getDepthString(depth) );

    return s;
}

std::string
Image::getDepthString(ImageBitDepthEnum depth)
{
    std::string s;

    switch (depth) {
    case eImageBitDepthByte:
        s += "8u";
        break;
    case eImageBitDepthShort:
        s += "16u";
        break;
    case eImageBitDepthHalf:
        s += "16f";
        break;
    case eImageBitDepthFloat:
        s += "32f";
        break;
    case eImageBitDepthNone:
        break;
    }

    return s;
}

bool
Image::isBitDepthConversionLossy(ImageBitDepthEnum from,
                                 ImageBitDepthEnum to)
{
    int sizeOfFrom = getSizeOfForBitDepth(from);
    int sizeOfTo = getSizeOfForBitDepth(to);

    return sizeOfTo < sizeOfFrom;
}


ImagePtr
Image::downscaleMipMap(const RectI & roi, unsigned int downscaleLevels) const
{
    // If we don't have to downscale or this is an OpenGL texture there's nothing to do
    if (downscaleLevels == 0 || getStorageMode() == eStorageModeGLTex) {
        return boost::const_pointer_cast<Image>(shared_from_this());
    }

    if (_imp->tiles.empty()) {
        return ImagePtr();
    }

    // The roi must be contained in the bounds of the image
    assert(_imp->originalBounds.contains(roi));
    if (!_imp->originalBounds.contains(roi)) {
        return ImagePtr();
    }

    RectI dstRoI  = roi.downscalePowerOfTwoSmallestEnclosing(downscaleLevels);
    ImagePtr mipmapImage;

    RectI previousLevelRoI = roi;
    ImageConstPtr previousLevelImage = shared_from_this();

    // The downscaling function only supports full rect format.
    // Copy this image to the appropriate format if necessary.
    if (previousLevelImage->_imp->bufferFormat == eImageBufferLayoutMonoChannelTiled) {
        InitStorageArgs args;
        args.bounds = roi;
        args.renderArgs = _imp->renderArgs;
        args.layer = previousLevelImage->_imp->layer;
        args.bitdepth = previousLevelImage->getBitDepth();
        args.proxyScale = previousLevelImage->getProxyScale();
        args.mipMapLevel = previousLevelImage->getMipMapLevel();
        ImagePtr tmpImg = Image::create(args);

        CopyPixelsArgs cpyArgs;
        cpyArgs.roi = roi;
        tmpImg->copyPixels(*this, cpyArgs);

        previousLevelImage = tmpImg;
    }


    // Build all the mipmap levels until we reach the one we are interested in
    for (unsigned int i = 0; i < downscaleLevels; ++i) {

        // Halve the smallest enclosing po2 rect as we need to render a minimum of the renderWindow
        RectI halvedRoI = previousLevelRoI.downscalePowerOfTwoSmallestEnclosing(1);

        // Allocate an image with half the size of the source image
        {
            InitStorageArgs args;
            args.bounds = halvedRoI;
            args.renderArgs = _imp->renderArgs;
            args.layer = previousLevelImage->_imp->layer;
            args.bitdepth = previousLevelImage->getBitDepth();
            args.proxyScale = previousLevelImage->getProxyScale();
            args.mipMapLevel = previousLevelImage->getMipMapLevel() + 1;
            mipmapImage = Image::create(args);
        }

        Image::CPUTileData srcTileData;
        getCPUTileData(previousLevelImage->_imp->tiles.begin()->second, &srcTileData);

        Image::CPUTileData dstTileData;
        getCPUTileData(mipmapImage->_imp->tiles.begin()->second, &dstTileData);

        ImagePrivate::halveImage((const void**)srcTileData.ptrs, srcTileData.nComps, srcTileData.bitDepth, srcTileData.tileBounds, dstTileData.ptrs, dstTileData.tileBounds);

        // Switch for next pass
        previousLevelRoI = halvedRoI;
        previousLevelImage = mipmapImage;
    } // for all downscale levels
    return mipmapImage;

} // downscaleMipMap

bool
Image::checkForNaNs(const RectI& roi)
{
    if (getBitDepth() != eImageBitDepthFloat) {
        return false;
    }
    if (getStorageMode() == eStorageModeGLTex) {
        return false;
    }

    bool hasNan = false;

    for (TileMap::iterator it = _imp->tiles.begin(); it != _imp->tiles.end(); ++it) {
        Image::CPUTileData tileData;
        getCPUTileData(it->second, &tileData);

        RectI tileRoi;
        roi.intersect(tileData.tileBounds, &tileRoi);
        hasNan |= _imp->checkForNaNs(tileData.ptrs, tileData.nComps, tileData.bitDepth, tileData.tileBounds, tileRoi);
    }
    return hasNan;

} // checkForNaNs


class MaskMixProcessor : public ImageMultiThreadProcessorBase
{
    Image::CPUTileData _srcTileData, _maskTileData, _dstTileData;
    double _mix;
    bool _maskInvert;
public:

    MaskMixProcessor(const TreeRenderNodeArgsPtr& renderArgs)
    : ImageMultiThreadProcessorBase(renderArgs)
    , _srcTileData()
    , _maskTileData()
    , _dstTileData()
    , _mix(0)
    , _maskInvert(false)
    {

    }

    virtual ~MaskMixProcessor()
    {
    }

    void setValues(const Image::CPUTileData& srcTileData,
                   const Image::CPUTileData& maskTileData,
                   const Image::CPUTileData& dstTileData,
                   double mix,
                   bool maskInvert)
    {
        _srcTileData = srcTileData;
        _maskTileData = maskTileData;
        _dstTileData = dstTileData;
        _mix = mix;
        _maskInvert = maskInvert;
    }

private:

    virtual ActionRetCodeEnum multiThreadProcessImages(const RectI& renderWindow, const TreeRenderNodeArgsPtr& renderArgs) OVERRIDE FINAL
    {
        ImagePrivate::applyMaskMixCPU((const void**)_srcTileData.ptrs, _srcTileData.tileBounds, _srcTileData.nComps, (const void**)_maskTileData.ptrs, _maskTileData.tileBounds, _dstTileData.ptrs, _dstTileData.bitDepth, _dstTileData.nComps, _mix, _maskInvert, _dstTileData.tileBounds, renderWindow, renderArgs);
        return eActionStatusOK;
    }
};

void
Image::applyMaskMix(const RectI& roi,
                    const ImagePtr& maskImg,
                    const ImagePtr& originalImg,
                    bool masked,
                    bool maskInvert,
                    float mix)
{
    // !masked && mix == 1: nothing to do
    if ( !masked && (mix == 1) ) {
        return;
    }

    // Mask must be alpha
    assert( !masked || !maskImg || maskImg->getLayer().getNumComponents() == 1 );

    if (getStorageMode() == eStorageModeGLTex) {

        GLImageStoragePtr originalImageTexture, maskTexture, dstTexture;
        if (originalImg) {
            assert(originalImg->getStorageMode() == eStorageModeGLTex);
            originalImageTexture = toGLImageStorage(originalImg->_imp->tiles.begin()->second.perChannelTile[0].buffer);
        }
        if (maskImg && masked) {
            assert(maskImg->getStorageMode() == eStorageModeGLTex);
            maskTexture = toGLImageStorage(maskImg->_imp->tiles.begin()->second.perChannelTile[0].buffer);
        }
        dstTexture = toGLImageStorage(_imp->tiles.begin()->second.perChannelTile[0].buffer);
        ImagePrivate::applyMaskMixGL(originalImageTexture, maskTexture, dstTexture, mix, maskInvert, roi);
        return;
    }

    // This function only works if original image and mask image are in full rect formats with the same bitdepth as output
    assert(!originalImg || (originalImg->getBufferFormat() != eImageBufferLayoutMonoChannelTiled && originalImg->getBitDepth() == getBitDepth()));
    assert(!maskImg || (maskImg->getBufferFormat() != eImageBufferLayoutMonoChannelTiled && maskImg->getBitDepth() == getBitDepth()));

    Image::CPUTileData srcImgData, maskImgData;
    if (originalImg) {
        getCPUTileData(originalImg->_imp->tiles.begin()->second, &srcImgData);
    }

    if (maskImg) {
        getCPUTileData(maskImg->_imp->tiles.begin()->second, &maskImgData);
        assert(maskImgData.nComps == 1);
    }


    for (TileMap::iterator it = _imp->tiles.begin(); it != _imp->tiles.end(); ++it) {

        Image::CPUTileData dstImgData;
        getCPUTileData(it->second, &dstImgData);

        RectI tileRoI;
        roi.intersect(dstImgData.tileBounds, &tileRoI);

        MaskMixProcessor processor(_imp->renderArgs);
        processor.setValues(srcImgData, maskImgData, dstImgData, mix, maskInvert);
        processor.setRenderWindow(tileRoI);
        processor.process();

    } // for each tile
} // applyMaskMix

bool
Image::canCallCopyUnProcessedChannels(const std::bitset<4> processChannels) const
{
    int numComp = getLayer().getNumComponents();

    if (numComp == 0) {
        return false;
    }
    if ( (numComp == 1) && processChannels[3] ) { // 1 component is alpha
        return false;
    } else if ( (numComp == 2) && processChannels[0] && processChannels[1] ) {
        return false;
    } else if ( (numComp == 3) && processChannels[0] && processChannels[1] && processChannels[2] ) {
        return false;
    } else if ( (numComp == 4) && processChannels[0] && processChannels[1] && processChannels[2] && processChannels[3] ) {
        return false;
    }

    return true;
}


class CopyUnProcessedProcessor : public ImageMultiThreadProcessorBase
{
    Image::CPUTileData _srcImgData, _dstImgData;
    std::bitset<4> _processChannels;
public:

    CopyUnProcessedProcessor(const TreeRenderNodeArgsPtr& renderArgs)
    : ImageMultiThreadProcessorBase(renderArgs)
    {

    }

    virtual ~CopyUnProcessedProcessor()
    {
    }

    void setValues(const Image::CPUTileData& srcImgData,
                   const Image::CPUTileData& dstImgData,
                   std::bitset<4> processChannels)
    {
        _srcImgData = srcImgData;
        _dstImgData = dstImgData;
        _processChannels = processChannels;
    }

private:

    virtual ActionRetCodeEnum multiThreadProcessImages(const RectI& renderWindow, const TreeRenderNodeArgsPtr& renderArgs) OVERRIDE FINAL
    {
        ImagePrivate::copyUnprocessedChannelsCPU((const void**)_srcImgData.ptrs, _srcImgData.tileBounds, _srcImgData.nComps, (void**)_dstImgData.ptrs, _dstImgData.bitDepth, _dstImgData.nComps, _dstImgData.tileBounds, _processChannels, renderWindow, renderArgs);
        return eActionStatusOK;
    }
};


void
Image::copyUnProcessedChannels(const RectI& roi,
                               const std::bitset<4> processChannels,
                               const ImagePtr& originalImg)
{

    if (!canCallCopyUnProcessedChannels(processChannels)) {
        return;
    }

    if (getStorageMode() == eStorageModeGLTex) {

        GLImageStoragePtr originalImageTexture, dstTexture;
        if (originalImg) {
            assert(originalImg->getStorageMode() == eStorageModeGLTex);
            originalImageTexture = toGLImageStorage(originalImg->_imp->tiles.begin()->second.perChannelTile[0].buffer);
        }

        dstTexture = toGLImageStorage(_imp->tiles.begin()->second.perChannelTile[0].buffer);

        RectI realRoi;
        roi.intersect(dstTexture->getBounds(), &realRoi);
        ImagePrivate::copyUnprocessedChannelsGL(originalImageTexture, dstTexture, processChannels, realRoi);
        return;
    }

    // This function only works if original  image is in full rect format with the same bitdepth as output
    assert(!originalImg || (originalImg->getBufferFormat() != eImageBufferLayoutMonoChannelTiled && originalImg->getBitDepth() == getBitDepth()));

    Image::CPUTileData srcImgData;
    if (originalImg) {
        getCPUTileData(originalImg->_imp->tiles.begin()->second, &srcImgData);
    }


    for (TileMap::iterator it = _imp->tiles.begin(); it != _imp->tiles.end(); ++it) {

        Image::CPUTileData dstImgData;
        getCPUTileData(it->second, &dstImgData);

        RectI tileRoI;
        roi.intersect(dstImgData.tileBounds, &tileRoI);

        CopyUnProcessedProcessor processor(_imp->renderArgs);
        processor.setValues(srcImgData, dstImgData, processChannels);
        processor.setRenderWindow(tileRoI);
        processor.process();

    } // for each tile
    
} // copyUnProcessedChannels

NATRON_NAMESPACE_EXIT;
