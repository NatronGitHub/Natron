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
#include "Engine/MultiThread.h"
#include "Engine/OSGLContext.h"
#include "Engine/OSGLFunctions.h"
#include "Engine/RectI.h"
#include "Engine/TreeRenderNodeArgs.h"
#include "Engine/ViewIdx.h"


NATRON_NAMESPACE_ENTER;





struct ImagePrivate
{
    // The rectangle where data are defined
    RectI bounds;

    // Each individual tile storage
    std::vector<Image::Tile> tiles;

    // The layer represented by this image
    ImageComponents layer;

    // The proxy scale of the image
    RenderScale proxyScale;

    // The mipmap level of the image
    unsigned int mipMapLevel;

    // Controls the cache access for the image
    CacheAccessModeEnum cachePolicy;

    // The buffer format
    ImageBufferLayoutEnum bufferFormat;

    // A pointer to another image that should be updated when this image dies.
    // Since tiled mono-channel format may not be ideal for most processing formats
    // you may provide a pointer to another mirror image with another format.
    // When this image is destroyed, it will update the mirror image.
    // If the mirror image is cached, it will in turn update the cache when destroyed.
    ImagePtr mirrorImage;

    // If mirrorImage is set, this is the portion to update in the mirror image.
    // This should be contained in mirrorImage
    RectI mirrorImageRoI;

    // This must be set if the cache policy is not none.
    // This will be used to prevent inserting in the cache part of images that had
    // their render aborted.
    TreeRenderNodeArgsPtr renderArgs;

    // Protects tilesInsertedInCache
    mutable QMutex tilesInsertedInCacheMutex;

    // When true, pushTilesToCacheIfNotAborted has been called and cannot be called again
    bool tilesInsertedInCache;

    ImagePrivate()
    : bounds()
    , tiles()
    , layer()
    , proxyScale(1.)
    , mipMapLevel(0)
    , cachePolicy(eCacheAccessModeNone)
    , bufferFormat(eImageBufferLayoutRGBAPackedFullRect)
    , mirrorImage()
    , mirrorImageRoI()
    , renderArgs()
    , tilesInsertedInCacheMutex()
    , tilesInsertedInCache(false)
    {

    }

    void initFromExternalBuffer(const Image::InitStorageArgs& args);

    /**
     * @brief Called in the destructor to insert tiles that were processed in the cache.
     **/
    void insertTilesInCache();

    /**
     * @brief Returns the tile corresponding to the pixel at position x,y or null if out of bounds
     **/
    const Image::Tile* getTile(int x, int y) const;

    /**
     * @brief Returns the number of tiles that fit in 1 line of the image
     **/
    int getNTilesPerLine() const;

    /**
     * @brief Returns a rectangle of tiles coordinates that span the given rectangle of pixel coordinates
     **/
    RectI getTilesCoordinates(const RectI& pixelCoordinates) const;

    /**
     * @brief Helper to copy from a untiled image to a tiled image
     **/
    void copyUntiledImageToTiledImage(const Image& fromImage, const Image::CopyPixelsArgs& args);

    /**
     * @brief Helper to copy from a tiled image to a untiled image
     **/
    void copyTiledImageToUntiledImage(const Image& fromImage, const Image::CopyPixelsArgs& args);

    /**
     * @brief Helper to copy from an untiled image to another untiled image
     **/
    void copyUntiledImageToUntiledImage(const Image& fromImage, const Image::CopyPixelsArgs& args);

    /**
     * @brief The main entry point to copy image portions.
     * The storage may vary as well as the number of components and the bitdepth.
     **/
    static void copyRectangle(const Image::Tile& fromTile,
                              StorageModeEnum fromStorage,
                              ImageBufferLayoutEnum fromLayout,
                              const Image::Tile& toTile,
                              StorageModeEnum toStorage,
                              ImageBufferLayoutEnum toLayout,
                              const Image::CopyPixelsArgs& args,
                              const TreeRenderNodeArgsPtr& renderArgs);

    /**
     * @brief If copying pixels from fromImage to toImage cannot be copied directly, this function
     * returns a temporary image that is suitable to copy then to the toImage.
     **/
    static ImagePtr checkIfCopyToTempImageIsNeeded(const Image& fromImage, const Image& toImage, const RectI& roi);

   

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
                                const void* srcBufPtrs[4],
                                int srcNComps,
                                ImageBitDepthEnum srcBitDepth,
                                const RectI& srcBounds,
                                void* dstBufPtrs[4],
                                int dstNComps,
                                ImageBitDepthEnum dstBitDepth,
                                const RectI& dstBounds,
                                const TreeRenderNodeArgsPtr& renderArgs);

    static void fillGL(const RectI & roi,
                       float r,
                       float g,
                       float b,
                       float a,
                       const GLCacheEntryPtr& texture);

    static void fillCPU(void* ptrs[4],
                        float r,
                        float g,
                        float b,
                        float a,
                        int nComps,
                        ImageBitDepthEnum bitDepth,
                        const RectI& bounds,
                        const RectI& roi,
                        const TreeRenderNodeArgsPtr& renderArgs);

    static void halveImage(const void* srcPtrs[4],
                           int nComps,
                           ImageBitDepthEnum bitdepth,
                           const RectI& srcBounds,
                           void* dstPtrs[4],
                           const RectI& dstBounds);

    static bool checkForNaNs(void* ptrs[4],
                             int nComps,
                             ImageBitDepthEnum bitdepth,
                             const RectI& bounds,
                             const RectI& roi);

    static void applyMaskMixGL(const GLCacheEntryPtr& originalTexture,
                               const GLCacheEntryPtr& maskTexture,
                               const GLCacheEntryPtr& dstTexture,
                               double mix,
                               bool invertMask,
                               const RectI& roi);

    static void applyMaskMixCPU(const void* originalImgPtrs[4],
                                const RectI& originalImgBounds,
                                int originalImgNComps,
                                const void* maskImgPtrs[4],
                                const RectI& maskImgBounds,
                                void* dstImgPtrs[4],
                                ImageBitDepthEnum dstImgBitDepth,
                                int dstImgNComps,
                                double mix,
                                bool invertMask,
                                const RectI& dstBounds,
                                const RectI& roi,
                                const TreeRenderNodeArgsPtr& renderArgs);

    static void copyUnprocessedChannelsGL(const GLCacheEntryPtr& originalTexture,
                                          const GLCacheEntryPtr& dstTexture,
                                          const std::bitset<4> processChannels,
                                          const RectI& roi);

    static void copyUnprocessedChannelsCPU(const void* originalImgPtrs[4],
                                           const RectI& originalImgBounds,
                                           int originalImgNComps,
                                           void* dstImgPtrs[4],
                                           ImageBitDepthEnum dstImgBitDepth,
                                           int dstImgNComps,
                                           const RectI& dstBounds,
                                           const std::bitset<4> processChannels,
                                           const RectI& roi,
                                           const TreeRenderNodeArgsPtr& renderArgs);
    
    
};

NATRON_NAMESPACE_EXIT;

#endif // NATRON_ENGINE_IMAGEPRIVATE_H
