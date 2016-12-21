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

NATRON_NAMESPACE_ENTER;

Image::Image()
: _imp(new ImagePrivate())
{

}

ImagePtr
Image::create(const InitStorageArgs& args)
{
    ImagePtr ret(new Image);
    ret->initializeStorage(args);
    return ret;
}

Image::~Image()
{
    // If there's a mirror image, update it
    if (_imp->mirrorImage) {
        CopyPixelsArgs cpyArgs;
        cpyArgs.roi = _imp->mirrorImageRoI;
        _imp->mirrorImage->copyPixels(*this, cpyArgs);
    }

    // Push tiles to cache if needed
    if (_imp->cachePolicy == eCacheAccessModeReadWrite ||
        _imp->cachePolicy == eCacheAccessModeWriteOnly) {
        _imp->insertTilesInCache();
    }
}

Image::InitStorageArgs::InitStorageArgs()
: bounds()
, storage(eStorageModeRAM)
, bitdepth(eImageBitDepthFloat)
, layer(ImageComponents::getRGBAComponents())
, components()
, cachePolicy(Image::eCacheAccessModeNone)
, bufferFormat(Image::eImageBufferLayoutRGBAPackedFullRect)
, mipMapLevel(0)
, isDraft(false)
, nodeHashKey(0)
, glContext()
, textureTarget(GL_TEXTURE_2D)
{
    // By default make all channels
    components[0] = components[1] = components[2] = components[3] = 1;
}

void
Image::initializeStorage(const Image::InitStorageArgs& args)
{
    CachePtr cache = appPTR->getCache();
    assert(cache);

    // Should be initialized once!
    assert(_imp->tiles.empty());
    if (!_imp->tiles.empty()) {
        throw std::bad_alloc();
    }

    // The bounds of the image must not be empty
    if (args.bounds.isNull()) {
        throw std::bad_alloc();
    }

    _imp->bounds = args.bounds;
    _imp->cachePolicy = args.cachePolicy;
    _imp->bufferFormat = args.bufferFormat;
    _imp->layer = args.layer;
    _imp->mipMapLevel = args.mipMapLevel;
    _imp->mirrorImage = args.mirrorImage;
    _imp->mirrorImageRoI = args.mirrorImageRoI;
    if (_imp->mirrorImage) {
        // If there's a mirror image, the portion to update must be contained in the mirror image
        // and in this image.
        if (!_imp->mirrorImage->getBounds().contains(_imp->mirrorImageRoI) ||
            !args.bounds.contains(_imp->mirrorImageRoI)) {
            throw std::bad_alloc();
        }
    }

    // OpenGL texture back-end only supports 32-bit float RGBA packed format.
    assert(args.storage != eStorageModeGLTex || (args.bufferFormat == eImageBufferLayoutRGBAPackedFullRect && args.bitdepth == eImageBitDepthFloat));
    if (args.storage == eStorageModeGLTex && (args.bufferFormat != eImageBufferLayoutRGBAPackedFullRect || args.bitdepth != eImageBitDepthFloat)) {
        throw std::bad_alloc();
    }

    // MMAP storage only support mono channel tiles.
    assert(args.storage != eStorageModeDisk || args.bufferFormat == eImageBufferLayoutMonoChannelTiled);
    if (args.storage == eStorageModeDisk && args.bufferFormat != eImageBufferLayoutMonoChannelTiled) {
        throw std::bad_alloc();
    }


    // For tiled layout, get the number of tiles in X and Y depending on the bounds and the tile zie.
    int nTilesHeight,nTilesWidth;
    int tileSize = 0;
    switch (args.bufferFormat) {
        case eImageBufferLayoutMonoChannelTiled: {
            // The size of a tile depends on the bitdepth
            tileSize = cache->getTileSizePx(args.bitdepth);
            nTilesHeight = std::ceil(_imp->bounds.height() / tileSize) * tileSize;
            nTilesWidth = std::ceil(_imp->bounds.width() / tileSize) * tileSize;
        }   break;
        case eImageBufferLayoutRGBACoplanarFullRect:
        case eImageBufferLayoutRGBAPackedFullRect:
            nTilesHeight = 1;
            nTilesWidth = 1;
            break;
    }


    int nTiles = nTilesWidth * nTilesHeight;
    assert(nTiles > 0);

    _imp->tiles.resize(nTiles);

    // Initialize each tile
    int tx = 0, ty = 0;
    for (int tile_i = 0; tile_i < nTiles; ++tile_i) {

        Image::Tile& tile = _imp->tiles[tile_i];

        const std::string& layerName = args.layer.getLayerName();

        // How many buffer should we make for a tile
        // A mono channel image should have one per channel
        std::vector<int> channelIndices;
        switch (args.bufferFormat) {
            case eImageBufferLayoutMonoChannelTiled: {

                for (int nc = 0; nc < args.layer.getNumComponents(); ++nc) {
                    if (args.components[nc]) {
                        channelIndices.push_back(nc);
                    }
                }
            }   break;
            case eImageBufferLayoutRGBACoplanarFullRect:
            case eImageBufferLayoutRGBAPackedFullRect:
                channelIndices.push_back(-1);
                break;
        }


        switch (args.bufferFormat) {
            case eImageBufferLayoutMonoChannelTiled:
                assert(tileSize != 0);
                // The tile bounds may not necessarily be a square if we are on the edge.
                tile.tileBounds.x1 = args.bounds.x1 + (tx * tileSize);
                tile.tileBounds.y1 = args.bounds.y1 + (ty * tileSize);
                tile.tileBounds.x2 = std::min(tile.tileBounds.x1 + tileSize, args.bounds.x2);
                tile.tileBounds.y2 = std::min(tile.tileBounds.y1 + tileSize, args.bounds.y2);
                break;
            case eImageBufferLayoutRGBACoplanarFullRect:
            case eImageBufferLayoutRGBAPackedFullRect:
                // Single tile that covers the entire image
                assert(nTiles == 1);
                tile.tileBounds = args.bounds;
                break;
        }


        tile.perChannelTile.resize(channelIndices.size());

        for (std::size_t c = 0; c < channelIndices.size(); ++c) {

            MonoChannelTile& thisChannelTile = tile.perChannelTile[c];
            thisChannelTile.channelIndex = channelIndices[c];

            std::string channelName;
            switch (args.bufferFormat) {
                case eImageBufferLayoutMonoChannelTiled: {
                    const std::vector<std::string>& compNames = args.layer.getComponentsNames();
                    assert(thisChannelTile.channelIndex >= 0 && thisChannelTile.channelIndex < (int)compNames.size());
                    channelName = layerName + "." + compNames[thisChannelTile.channelIndex];
                }   break;
                case eImageBufferLayoutRGBACoplanarFullRect:
                case eImageBufferLayoutRGBAPackedFullRect:
                    channelName = layerName;
                    break;
            }

            // Make-up the key for this tile 
            ImageTileKeyPtr key(new ImageTileKey(args.nodeHashKey,
                                                 channelName,
                                                 args.mipMapLevel,
                                                 args.isDraft,
                                                 args.bitdepth,
                                                 tx,
                                                 ty));

            // The entry in the cache
            CacheEntryBasePtr cacheEntry;

            // If the entry was not in the cache but we want to cache it, this will lock the hash so that another thread
            // does not compute the same tile.
            CacheEntryLockerPtr entryLocker;

            // If the entry wants to be cached but we don't want to read from the cache
            // we must remove from the cache any entry that already exists at the given hash.
            if (args.cachePolicy == eCacheAccessModeWriteOnly) {
                bool isCached = cache->get(key, &cacheEntry, &entryLocker);
                assert((isCached && cacheEntry && !entryLocker) || (!isCached && entryLocker && !cacheEntry));
                if (isCached) {
                    assert(cacheEntry && !entryLocker);
                    cache->removeEntry(cacheEntry);
                    cacheEntry.reset();
                }
            }

            // Look in the cache
            bool isCached = false;
            if (args.cachePolicy == eCacheAccessModeReadWrite || args.cachePolicy == eCacheAccessModeWriteOnly) {

                for (int draft_i = 0; draft_i < 2; ++draft_i) {

                    const bool useDraft = (const bool)draft_i;

                    ImageTileKeyPtr keyToReadCache(new ImageTileKey(args.nodeHashKey,
                                                                    channelName,
                                                                    args.mipMapLevel,
                                                                    useDraft,
                                                                    args.bitdepth,
                                                                    tx,
                                                                    ty));

                    // In eCacheAccessModeWriteOnly mode, we already accessed the cache. If we got an entry locker because the entry
                    // was not cached, do not read a second time.
                    if (!entryLocker) {
                        isCached = cache->get(keyToReadCache, &cacheEntry, &entryLocker);
                        assert((isCached && cacheEntry && !entryLocker) || (!isCached && entryLocker && !cacheEntry));
                    }
                    if (isCached) {
                        // Found in cache, check if the key is really equal to this key
                        if (keyToReadCache->equals(*cacheEntry->getKey())) {
                            MemoryBufferedCacheEntryBasePtr isBufferedEntry = boost::dynamic_pointer_cast<MemoryBufferedCacheEntryBase>(cacheEntry);
                            assert(isBufferedEntry);
                            thisChannelTile.buffer = isBufferedEntry;
                            thisChannelTile.isCached = true;

                            // We found a cache entry, don't continue to look for a tile computed in draft mode.
                            break;
                        }
                    }
                }
            }
            
            if (!isCached) {

                // If we got the locker object, we are the only thread computing this tile.
                assert(args.cachePolicy == eCacheAccessModeNone || entryLocker);

                thisChannelTile.isCached = false;

                // Make the hash locker live as long as this image will live
                thisChannelTile.entryLocker = entryLocker;
                
                boost::shared_ptr<AllocateMemoryArgs> allocArgs;
                MemoryBufferedCacheEntryBasePtr entryBuffer;
                // Allocate a new entry
                switch (args.storage) {
                    case eStorageModeDisk: {
                        MemoryMappedCacheEntryPtr buffer(new MemoryMappedCacheEntry(cache));
                        entryBuffer = buffer;
                        boost::shared_ptr<MMAPAllocateMemoryArgs> a(new MMAPAllocateMemoryArgs());
                        a->bitDepth = args.bitdepth;
                        allocArgs = a;
                    }   break;
                    case eStorageModeGLTex: {
                        GLCacheEntryPtr buffer(new GLCacheEntry(cache));
                        entryBuffer = buffer;
                        boost::shared_ptr<GLAllocateMemoryArgs> a(new GLAllocateMemoryArgs());
                        a->textureTarget = args.textureTarget;
                        a->glContext = args.glContext;
                        a->bounds = tile.tileBounds;
                        a->bitDepth = args.bitdepth;
                        allocArgs = a;
                    }   break;
                    case eStorageModeRAM: {
                        RAMCacheEntryPtr buffer(new RAMCacheEntry(cache));
                        entryBuffer = buffer;
                        boost::shared_ptr<RAMAllocateMemoryArgs> a(new RAMAllocateMemoryArgs());
                        a->bitDepth = args.bitdepth;
                        a->bounds = tile.tileBounds;

                        if (thisChannelTile.channelIndex == -1) {
                            a->numComponents = (std::size_t)args.layer.getNumComponents();
                        } else {
                            a->numComponents = 1;
                        }
                        allocArgs = a;
                    }   break;
                    case eStorageModeNone:
                        assert(false);
                        throw std::bad_alloc();
                        break;
                }
                assert(allocArgs && entryBuffer);

                entryBuffer->setKey(key);

                // Allocate the memory for the tile.
                // This may throw a std::bad_alloc
                entryBuffer->allocateMemory(*allocArgs);
                
            } // !isCached


        } // for each channel

        // Increment tile coords
        if (tx == nTilesWidth - 1) {
            tx = 0;
            ++ty;
        } else {
            ++tx;
        }
    } // for each tile

} // initializeStorage

Image::CopyPixelsArgs::CopyPixelsArgs()
: roi()
, conversionChannel(0)
, alphaHandling(Image::eAlphaChannelHandlingFillFromChannel)
, monoConversion(Image::eMonoToPackedConversionCopyToChannelAndLeaveOthers)
, srcColorspace(eViewerColorSpaceLinear)
, dstColorspace(eViewerColorSpaceLinear)
, unPremultIfNeeded(false)
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
    if (!other._imp->bounds.intersect(args.roi, &roi)) {
        return;
    }
    if (!_imp->bounds.intersect(args.roi, &roi)) {
        return;
    }

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

Image::ImageBufferLayoutEnum
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
    assert(!_imp->tiles[0].perChannelTile.empty() && _imp->tiles[0].perChannelTile[0].buffer);
    return _imp->tiles[0].perChannelTile[0].buffer->getStorageMode();
}

const RectI&
Image::getBounds() const
{
    return _imp->bounds;
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


const ImageComponents&
Image::getComponents() const
{
    return _imp->layer;
}


ImageBitDepthEnum
Image::getBitDepth() const
{
    if (_imp->tiles.empty()) {
        return eImageBitDepthNone;
    }
    assert(!_imp->tiles[0].perChannelTile.empty() && _imp->tiles[0].perChannelTile[0].buffer);
    return _imp->tiles[0].perChannelTile[0].buffer->getBitDepth();
}

void
Image::getCPUTileData(const Image::Tile& tile,
                      void* ptrs[4],
                      RectI *tileBounds,
                      ImageBitDepthEnum *bitDepth,
                      int *nComps) const
{
    memset(ptrs, 0, sizeof(void*) * 4);
    *nComps = 0;
    *bitDepth = eImageBitDepthNone;
    
    for (std::size_t i = 0; i < tile.perChannelTile.size(); ++i) {
        RAMCacheEntryPtr fromIsRAMBuffer = toRAMCacheEntry(tile.perChannelTile[i].buffer);
        MemoryMappedCacheEntryPtr fromIsMMAPBuffer = toMemoryMappedCacheEntry(tile.perChannelTile[i].buffer);

        if (!fromIsMMAPBuffer || !fromIsRAMBuffer) {
            continue;
        }
        if (i == 0) {
            if (fromIsRAMBuffer) {
                ptrs[0] = fromIsRAMBuffer->getData();
                *tileBounds = fromIsRAMBuffer->getBounds();
                *bitDepth = fromIsRAMBuffer->getBitDepth();
                *nComps = fromIsRAMBuffer->getNumComponents();

                if (_imp->bufferFormat == Image::eImageBufferLayoutRGBACoplanarFullRect) {
                    // Coplanar requires offsetting
                    assert(tile.perChannelTile.size() == 1);
                    std::size_t planeSize = *nComps * tileBounds->area() * getSizeOfForBitDepth(*bitDepth);
                    if (*nComps > 1) {
                        ptrs[1] = (char*)ptrs[0] + planeSize;
                        if (*nComps > 2) {
                            ptrs[2] = (char*)ptrs[1] + planeSize;
                            if (*nComps > 3) {
                                ptrs[3] = (char*)ptrs[2] + planeSize;
                            }
                        }
                    }
                }
            } else {
                ptrs[0] = fromIsMMAPBuffer->getData();
                *tileBounds = fromIsMMAPBuffer->getBounds();
                *bitDepth = fromIsMMAPBuffer->getBitDepth();
                // MMAP based storage only support mono channel images
                *nComps = 1;
            }
        } else {
            int channelIndex = tile.perChannelTile[i].channelIndex;
            assert(channelIndex >= 0 && channelIndex < 4);
            if (fromIsRAMBuffer) {
                ptrs[channelIndex] = fromIsRAMBuffer->getData();
            } else {
                ptrs[channelIndex] = fromIsMMAPBuffer->getData();
            }
        }
    } // for each channel
}


bool
Image::getTileAt(int tileIndex, Image::Tile* tile) const
{
    if (!tile || tileIndex < 0 || tileIndex > (int)_imp->tiles.size()) {
        return false;
    }
    *tile = _imp->tiles[tileIndex];
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
        GLCacheEntryPtr glEntry = toGLCacheEntry(_imp->tiles[0].perChannelTile[0].buffer);
        _imp->fillGL(roi, r, g, b, a, glEntry);
        return;
    }



    for (std::size_t tile_i = 0; tile_i < _imp->tiles.size(); ++tile_i) {
        
        void* ptrs[4];
        RectI tileBounds;
        ImageBitDepthEnum bitDepth;
        int nComps;

        getCPUTileData(_imp->tiles[tile_i], ptrs, &tileBounds, &bitDepth, &nComps);
        RectI tileRoI;
        roi.intersect(tileBounds, &tileRoI);

        _imp->fillCPU(ptrs, r, g, b, a, nComps, bitDepth, tileBounds, tileRoI);
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
Image::getFormatString(const ImageComponents& comps,
                       ImageBitDepthEnum depth)
{
    std::string s = comps.getLayerName() + '.' + comps.getComponentsGlobalName();

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
    assert(_imp->bounds.contains(roi));
    if (!_imp->bounds.contains(roi)) {
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
        args.layer = previousLevelImage->_imp->layer;
        args.bitdepth = previousLevelImage->getBitDepth();
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
            args.layer = previousLevelImage->_imp->layer;
            args.bitdepth = previousLevelImage->getBitDepth();
            args.mipMapLevel = previousLevelImage->getMipMapLevel() + 1;
            mipmapImage = Image::create(args);
        }

        void* srcPtrs[4];
        RectI srcBounds;
        ImageBitDepthEnum srcBitdepth;
        int srcNComps;
        getCPUTileData(previousLevelImage->_imp->tiles[0], srcPtrs, &srcBounds, &srcBitdepth, &srcNComps);


        void* dstPtrs[4];
        RectI dstBounds;
        ImageBitDepthEnum dstBitDepth;
        int dstNComps;
        getCPUTileData(mipmapImage->_imp->tiles[0], dstPtrs, &dstBounds, &dstBitDepth, &dstNComps);

        ImagePrivate::halveImage((const void**)srcPtrs, srcNComps, srcBitdepth, srcBounds, dstPtrs, dstBounds);

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

    for (std::size_t i = 0; i < _imp->tiles.size(); ++i) {
        void* ptrs[4];
        RectI bounds;
        ImageBitDepthEnum bitDepth;
        int nComps;
        getCPUTileData(_imp->tiles[i], ptrs, &bounds, &bitDepth, &nComps);

        RectI tileRoi;
        roi.intersect(bounds, &tileRoi);
        hasNan |= _imp->checkForNaNs(ptrs, nComps, bitDepth, bounds, tileRoi);
    }
    return hasNan;

} // checkForNaNs


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
    assert( !masked || !maskImg || maskImg->getComponents() == ImageComponents::getAlphaComponents() );

    if (getStorageMode() == eStorageModeGLTex) {

        GLCacheEntryPtr originalImageTexture, maskTexture, dstTexture;
        if (originalImg) {
            assert(originalImg->getStorageMode() == eStorageModeGLTex);
            originalImageTexture = toGLCacheEntry(originalImg->_imp->tiles[0].perChannelTile[0].buffer);
        }
        if (maskImg && masked) {
            assert(maskImg->getStorageMode() == eStorageModeGLTex);
            maskTexture = toGLCacheEntry(maskImg->_imp->tiles[0].perChannelTile[0].buffer);
        }
        dstTexture = toGLCacheEntry(_imp->tiles[0].perChannelTile[0].buffer);
        ImagePrivate::applyMaskMixGL(originalImageTexture, maskTexture, dstTexture, mix, maskInvert, roi);
        return;
    }

    // This function only works if original image and mask image are in full rect formats with the same bitdepth as output
    assert(!originalImg || (originalImg->getBufferFormat() != eImageBufferLayoutMonoChannelTiled && originalImg->getBitDepth() == getBitDepth()));
    assert(!maskImg || (maskImg->getBufferFormat() != eImageBufferLayoutMonoChannelTiled && maskImg->getBitDepth() == getBitDepth()));

    void* origImgPtrs[4] = {0, 0, 0, 0};
    RectI origImgBounds;
    ImageBitDepthEnum origImgBitdepth = eImageBitDepthFloat;
    int origImgNComps = 0;
    if (originalImg) {
        getCPUTileData(originalImg->_imp->tiles[0], origImgPtrs, &origImgBounds, &origImgBitdepth, &origImgNComps);
    }

    void* maskImgPtrs[4] = {0, 0, 0, 0};
    RectI maskImgBounds;
    ImageBitDepthEnum maskImgBitdepth = eImageBitDepthFloat;
    int maskImgNComps = 0;
    if (originalImg) {
        getCPUTileData(maskImg->_imp->tiles[0], maskImgPtrs, &maskImgBounds, &maskImgBitdepth, &maskImgNComps);
    }
    assert(maskImgNComps == 1);

    for (std::size_t tile_i; tile_i < _imp->tiles.size(); ++tile_i) {

    } // for each tile




    int srcNComps = originalImg ? (int)originalImg->getComponentsCount() : 0;
    switch (srcNComps) {
        case 1:
            applyMaskMixForSrcComponents<1>(realRoI, maskImg, originalImg, masked, maskInvert, mix);
            break;
        case 2:
            applyMaskMixForSrcComponents<2>(realRoI, maskImg, originalImg, masked, maskInvert, mix);
            break;
        case 3:
            applyMaskMixForSrcComponents<3>(realRoI, maskImg, originalImg, masked, maskInvert, mix);
            break;
        case 4:
            applyMaskMixForSrcComponents<4>(realRoI, maskImg, originalImg, masked, maskInvert, mix);
            break;
        default:
            break;
    }
} // applyMaskMix



NATRON_NAMESPACE_EXIT;
