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

#ifndef NATRON_ENGINE_IMAGEPRIVATE_H
#define NATRON_ENGINE_IMAGEPRIVATE_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Engine/AppManager.h"
#include "Engine/EngineFwd.h"
#include "Engine/Cache.h"
#include "Engine/CacheEntryBase.h"
#include "Engine/CacheEntryKeyBase.h"
#include "Engine/GPUContextPool.h"
#include "Engine/Image.h"
#include "Engine/OSGLContext.h"
#include "Engine/OSGLFunctions.h"
#include "Engine/RectI.h"
#include "Engine/ViewIdx.h"


NATRON_NAMESPACE_ENTER;

struct MonoChannelTile
{
    // A pointer to the internal storage
    MemoryBufferedCacheEntryBasePtr buffer;

    // Used when working with the cache
    // to ensure a single thread computed this tile
    CacheEntryLockerPtr entryLocker;

    // The index of the channel 0 <= channel <= 3
    // if mono channel.
    // If not mon channel this is -1
    int channelIndex;

    // Was this found in the cache ?
    bool isCached;

};

struct ImageTile
{
    // Each ImageTile internally holds a pointer to a mono-channel tile in the cache
    std::vector<MonoChannelTile> perChannelTile;

    // The bounds covered by this tile
    RectI tileBounds;

};

struct ImagePrivate
{
    // The rectangle where data are defined
    RectI bounds;

    // Each individual tile storage
    std::vector<ImageTile> tiles;

    // The layer represented by this image
    ImageComponents layer;

    // The mipmap level of the image
    unsigned int mipMapLevel;

    // Controls the cache access for the image
    Image::CacheAccessModeEnum cachePolicy;

    // The buffer format
    Image::ImageBufferLayoutEnum bufferFormat;

    // A pointer to another image that should be updated when this image dies.
    // Since tiled mono-channel format may not be ideal for most processing formats
    // you may provide a pointer to another mirror image with another format.
    // When this image is destroyed, it will update the mirror image.
    // If the mirror image is cached, it will in turn update the cache when destroyed.
    ImagePtr mirrorImage;

    // If mirrorImage is set, this is the portion to update in the mirror image.
    // This should be contained in mirrorImage
    RectI mirrorImageRoI;

    ImagePrivate()
    : bounds()
    , tiles()
    , layer()
    , mipMapLevel(0)
    , cachePolicy(Image::eCacheAccessModeNone)
    , bufferFormat(Image::eImageBufferLayoutRGBAPackedFullRect)
    , mirrorImage()
    , mirrorImageRoI()
    {

    }

    /**
     * @brief Called in the destructor to insert tiles that were processed in the cache.
     **/
    void insertTilesInCache();

    /**
     * @brief Returns the tile corresponding to the pixel at position x,y or null if out of bounds
     **/
    const ImageTile* getTile(int x, int y) const;

    /**
     * @brief Returns the number of tiles that fit in 1 line of the image
     **/
    int getNTilesPerLine() const;

    /**
     * @brief Returns a rectangle of tiles coordinates that span the given rectangle of pixel coordinates
     **/
    RectI getTilesCoordinates(const RectI& pixelCoordinates) const;


    /**
     * @brief The main entry point to copy image portions.
     * The storage may vary as well as the number of components and the bitdepth.
     **/
    static void copyRectangle(const MemoryBufferedCacheEntryBasePtr& from,
                              const int fromChannelIndex,
                              Image::ImageBufferLayoutEnum fromLayout,
                              const MemoryBufferedCacheEntryBasePtr& to,
                              const int toChannelIndex,
                              Image::ImageBufferLayoutEnum toLayout,
                              const RectI& roi);

   

    /**
     * @brief This function can be used to convert CPU buffers with different
     * bitdepth or components.
     *
     * @param renderWindow The rectangle to convert
     *
     * @param srcColorSpace Input data will be taken to be in this color-space
     *
     * @param dstColorSpace Output data will be converted to this color-space.
     *
     *
     * @param conversionChannel A value in 0 <= index < 4 indicating the channel to use 
     * for the AlphaChannelHandlingEnum and MonoToPackedConversionEnum flags.
     *
     *
     * @param useAlpha0 If true, when creating an alpha channel the alpha will be filled with 0, otherwise filled with 1.
     *
     * @param requiresUnpremult If true, if a component conversion from RGBA to RGB happens
     * the RGB channels will be divided by the alpha channel when copied to the output image
     **/
    static void convertCPUImage(const RectI & renderWindow,
                                ViewerColorSpaceEnum srcColorSpace,
                                ViewerColorSpaceEnum dstColorSpace,
                                bool requiresUnpremult,
                                int conversionChannel,
                                Image::AlphaChannelHandlingEnum alphaHandling,
                                Image::MonoToPackedConversionEnum monoConversion,
                                const void* srcBuf,
                                int srcNComps,
                                ImageBitDepthEnum srcBitDepth,
                                const RectI& srcBounds,
                                void* dstBuf,
                                int dstNComps,
                                ImageBitDepthEnum dstBitDepth,
                                const RectI& dstBounds);
    
    
};

NATRON_NAMESPACE_EXIT;

#endif // NATRON_ENGINE_IMAGEPRIVATE_H
