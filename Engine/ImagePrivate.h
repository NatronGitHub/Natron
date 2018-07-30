/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
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

#include "Global/Macros.h"

#include <map>
#include <QMutex>
#include "Engine/AppManager.h"
#include "Engine/Cache.h"
#include "Engine/CacheEntryBase.h"
#include "Engine/CacheEntryKeyBase.h"
#include "Engine/EffectInstance.h"
#include "Engine/GPUContextPool.h"
#include "Engine/Image.h"
#include "Engine/ImageStorage.h"
#include "Engine/ImageCacheKey.h"
#include "Engine/ImageCacheEntry.h"
#include "Engine/MultiThread.h"
#include "Engine/OSGLContext.h"
#include "Engine/OSGLFunctions.h"
#include "Engine/RectI.h"
#include "Engine/TimeValue.h"
#include "Engine/ViewIdx.h"

#include "Engine/EngineFwd.h"


NATRON_NAMESPACE_ENTER

struct ImagePrivate
{
    Image* _publicInterface;

    // The rectangle where data are defined
    RectI originalBounds;

    // Each individual channel storage if mono channel.
    // If not mono channel only the first buffer contains all channels
    ImageStorageBasePtr channels[4];

    // The plane represented by this image
    ImagePlaneDesc plane;

    // The proxy scale of the image
    RenderScale proxyScale;

    // The mipmap level of the image
    unsigned int mipMapLevel;

    // Controls the cache access for the image
    CacheAccessModeEnum cachePolicy;

    // The cache interface
    ImageCacheEntryPtr cacheEntry;

    // The buffer format
    ImageBufferLayoutEnum bufferFormat;

    // This must be set if the cache policy is not none.
    // This will be used to prevent inserting in the cache part of images that had
    // their render aborted.
    EffectInstanceWPtr renderClone;

    // The bitdepth of the image
    ImageBitDepthEnum bitdepth;

    // The image storage type
    StorageModeEnum storage;

    // Protects tilesAllocated
    QMutex tilesAllocatedMutex;

    // If true, tiles are assumed to be allocated, otherwise only cached tiles hold memory
    bool tilesAllocated;


    ImagePrivate(Image *publicInterface)
    : _publicInterface(publicInterface)
    , originalBounds()
    , channels()
    , plane()
    , proxyScale(1.)
    , mipMapLevel(0)
    , cachePolicy(eCacheAccessModeNone)
    , cacheEntry()
    , bufferFormat(eImageBufferLayoutRGBAPackedFullRect)
    , renderClone()
    , bitdepth(eImageBitDepthNone)
    , storage(eStorageModeNone)
    , tilesAllocatedMutex()
    , tilesAllocated(false)
    {

    }

    ActionRetCodeEnum init(const Image::InitStorageArgs& args);

    ActionRetCodeEnum initFromExternalBuffer(const Image::InitStorageArgs& args);

    ActionRetCodeEnum initAndFetchFromCache(const Image::InitStorageArgs& args);

    static void getCPUDataInternal(const RectI& bounds,
                                   int nComps,
                                   const ImageStorageBasePtr storage[4],
                                   ImageBitDepthEnum depth,
                                   ImageBufferLayoutEnum format,
                                   Image::CPUData* data);

    /**
     * @brief The main entry point to copy image portions.
     * The storage may vary as well as the number of components and the bitdepth.
     **/
    static ActionRetCodeEnum copyPixelsInternal(const ImagePrivate* fromImage,
                                   ImagePrivate* toImage,
                                   const Image::CopyPixelsArgs& args,
                                   const EffectInstancePtr& renderClone);

    /**
     * @brief If copying pixels from fromImage to toImage cannot be copied directly, this function
     * returns a temporary image that is suitable to copy then to the toImage.
     **/
    static ActionRetCodeEnum checkIfCopyToTempImageIsNeeded(const Image& fromImage, const Image& toImage, const RectI& roi, ImagePtr* outImage);
    
    
    
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
    static ActionRetCodeEnum convertCPUImage(const RectI & renderWindow,
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
                                             const EffectInstancePtr& renderClone);

    static void fillGL(const RectI & roi,
                       float r,
                       float g,
                       float b,
                       float a,
                       const GLTexturePtr& texture,
                       const OSGLContextPtr& glContext);

    static ActionRetCodeEnum fillCPU(void* ptrs[4],
                                     float r,
                                     float g,
                                     float b,
                                     float a,
                                     int nComps,
                                     ImageBitDepthEnum bitDepth,
                                     const RectI& bounds,
                                     const RectI& roi,
                                     const EffectInstancePtr& renderClone);

    static ActionRetCodeEnum halveImage(const void* srcPtrs[4],
                                        int nComps,
                                        ImageBitDepthEnum bitdepth,
                           const RectI& srcBounds,
                           void* dstPtrs[4],
                           const RectI& dstBounds,
                           const EffectInstancePtr& renderClone);

    static ActionRetCodeEnum checkForNaNs(void* ptrs[4],
                                          int nComps,
                                          ImageBitDepthEnum bitdepth,
                                          const RectI& bounds,
                                          const RectI& roi,
                                          const EffectInstancePtr& effect,
                                          bool* foundNan);

    static void applyMaskMixGL(const GLImageStoragePtr& originalTexture,
                               const GLImageStoragePtr& maskTexture,
                               const GLImageStoragePtr& dstTexture,
                               double mix,
                               bool invertMask,
                               const RectI& roi);

    static ActionRetCodeEnum applyMaskMixCPU(const void* originalImgPtrs[4],
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
                                const EffectInstancePtr& renderClone);

    static void copyUnprocessedChannelsGL(const GLImageStoragePtr& originalTexture,
                                          const GLImageStoragePtr& dstTexture,
                                          const std::bitset<4> processChannels,
                                          const RectI& roi);

    static ActionRetCodeEnum copyUnprocessedChannelsCPU(const void* originalImgPtrs[4],
                                           const RectI& originalImgBounds,
                                           int originalImgNComps,
                                           void* dstImgPtrs[4],
                                           ImageBitDepthEnum dstImgBitDepth,
                                           int dstImgNComps,
                                           const RectI& dstBounds,
                                                        const std::bitset<4> processChannels,
                                                        const RectI& roi,
                                                        const EffectInstancePtr& renderClone);

    static void copyGLTexture(const GLTexturePtr& from,
                              const GLTexturePtr& to,
                              const RectI& roi,
                              const OSGLContextPtr& glContext);


    static ActionRetCodeEnum convertRGBAPackedCPUBufferToGLTexture(const float* srcData,
                                                                   const RectI& srcBounds,
                                                                   int srcNComps,
                                                                   const RectI& roi,
                                                                   const GLTexturePtr& outTexture,
                                                                   const OSGLContextPtr& glContext);

    static ActionRetCodeEnum convertGLTextureToRGBAPackedCPUBuffer(const GLTexturePtr& texture,
                                                                   const RectI& roi,
                                                                   float* outBuffer,
                                                                   const RectI& dstBounds,
                                                                   int dstNComps,
                                                                   const OSGLContextPtr& glContext);

    static ActionRetCodeEnum applyCPUPixelShader(const RectI& roi,
                                                 const void* customData, 
                                                 const EffectInstancePtr& renderClone,
                                                 const Image::CPUData& cpuData,
                                                 void(*func)());

    ActionRetCodeEnum applyCPUPixelShaderForDepth(const RectI& roi,
                                                 const void* customData,
                                                 void(*func)());
    

};

NATRON_NAMESPACE_EXIT

#endif // NATRON_ENGINE_IMAGEPRIVATE_H
