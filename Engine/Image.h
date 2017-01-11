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

#ifndef NATRON_ENGINE_IMAGE_H
#define NATRON_ENGINE_IMAGE_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <bitset>
#include <list>

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/scoped_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#endif

#include "Global/GlobalDefines.h"

#include "Global/GLIncludes.h"
#include "Engine/ImageComponents.h"
#include "Engine/RectI.h"
#include "Engine/ViewIdx.h"

#include "Engine/EngineFwd.h"

NATRON_NAMESPACE_ENTER;

/**
 * @brief An image in Natron is a view one or multiple buffers that may be cached.
 * An image may have the following forms: tiled, single rectangle, packed buffer or mono-channel...
 * This is described in the ImageBufferLayoutEnum enum.
 *
 * The underlying storage of the image may be requested by using the InitStorageArgs struct as well as the bounds
 * and more details. At the very least the bounds must be specified.
 *
 * If the image fails to create, an std::bad_alloc exception will be thrown.
 * 
 * The image may use the cache to recover underlying tiles that were already computed. To specify whether this Image
 * should interact with the cache or not you may use the CacheAccessModeEnum.
 * By default an image does not use the cache.
 *
 * If the image uses the cache, then when the destructor is invoked, all underlying tiles that were not cached will be pushed
 * to the cache so that other threads may re-use it.
 *
 * Since tiled mono-channel format may not be ideal for most processing formats, you may create a temporary image that takes in
 * parameter another image that itself is mono-channel and tiled. 
 * When the temporary image is destroyed, it will update the other mirror image, which in turn when destroyed will update the cache.
 *
 **/
struct ImagePrivate;
class Image
: public boost::enable_shared_from_this<Image>
{
    /**
     * @brief Ctor. Make a new empty image. This is private, the static create() function should be used instead.
     **/
    Image();

public:

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
        
    };

    struct Tile
    {
        // Each Tile internally holds a pointer to a mono-channel tile in the cache
        std::vector<MonoChannelTile> perChannelTile;

        // The bounds covered by this tile
        RectI tileBounds;
        
    };
    


    struct InitStorageArgs
    {
        // The bounds of the image. This will internally set the required number of tiles
        //
        // Must be set
        RectI bounds;

        // The storage for the image.
        //
        // - If storage is an OpenGL texture, then it is expected that the buffer layout is set to
        // eImageBufferLayoutRGBAPackedFullRect and bitdepth to eImageBitDepthFloat
        //
        // - If storage is on disk (using MMAP), then it is expected that the buffer layout is set to
        // eImageBufferLayoutMonoChannelTiled as this is the only format supported by MMAP.
        //
        // Default - eStorageModeRAM
        StorageModeEnum storage;

        // The bitdepth of the image. Internally this will control how many tiles this image spans.
        // For OpenGL textures, only float RGBA textures are supported for now.
        //
        // Default - eImageBitDepthFloat
        ImageBitDepthEnum bitdepth;

        // The layer represented by this image.
        //
        // Default - Color.RGBA layer
        ImageComponents layer;

        // Components represented by the image.
        // This is only used for a mono channel buffered layout
        // it allows to only create storage for specific channels.
        // The relevant bits are only those that are in the range of layer.getNumComponents()
        //
        // Default - All bits set to 1
        std::bitset<4> components;

        // Whether or not the initializeStorage() may look for already existing tiles in the cache
        //
        // Default: eCacheAccessModeNone
        CacheAccessModeEnum cachePolicy;

        // This must be set if the cache policy is not none.
        // This will be used to prevent inserting in the cache part of images that had
        // their render aborted.
        // Also any of the algorithms used by this image (fill, copyUnprocessChannels, etc..)
        // will use this to check if the render has been aborted and cancel early the processing
        // if needed.
        TreeRenderNodeArgsPtr renderArgs;

        // Indicates the desired buffer format for the image. This defaults to eImageBufferLayoutRGBAPackedFullRect
        //
        // Default - eImageBufferLayoutRGBAPackedFullRect
        ImageBufferLayoutEnum bufferFormat;
        
        // The scale of the image: This is the scale of the render: for a proxy render, this should be (1,1)
        // and for a proxy render, the scale to convert from the full format to proxy format
        //
        // Default - (1,1)
        RenderScale proxyScale;

        // This is the mip map level of the image: it is a scale factor applied to the proxyScale.
        // For a mipMapLevel of 0, the factor is 1, level 1=0.5, level 2= 0.25, etc...
        //
        // Default - 0
        unsigned int mipMapLevel;

        // Is this image used to perform draft computations ? (i.e that were computed with low samples)
        // This is relevant only if the image is cached
        //
        // Default - false
        bool isDraft;

        // In order to be cached, the tiles of the image need to know the node hash of the node they correspond to.
        //
        // Default - 0
        U64 nodeHashKey;

        // If the storage is set to OpenGL texture, this is the OpenGL context to which the texture belongs to.
        //
        // Default - NULL
        OSGLContextPtr glContext;

        // The texture target, only relevant when storage is set to an OpenGL texture
        //
        // Default - GL_TEXTURE_2D
        U32 textureTarget;

        // If set, the internal storage of the image will be set to this buffer instead of allocating a new one
        // In this case, this is expected that the image is single tiled with bounds exactly covering the bounds
        // of the memory buffer and storage, bitdepth etc... matching the provided buffer properties.
        //
        // Default - NULL
        MemoryBufferedCacheEntryBasePtr externalBuffer;

        InitStorageArgs();
    };

private:

    void initializeStorage(const InitStorageArgs& args);

public:

    /**
     * @brief Make an image with the given storage parameters. 
     * This may fail, in which case a std::bad_alloc exception is thrown. 
     **/
    static ImagePtr create(const InitStorageArgs& args);

    /**
      * @brief Releases the storage used by this image and if needed push the pixels to the cache.
     **/
    virtual ~Image();

    /**
     * @brief Returns the internal buffer formating
     **/
    ImageBufferLayoutEnum getBufferFormat() const;

    /**
     * @brief Returns the internal storage of the image that was supplied in the create() function.
     **/
    StorageModeEnum getStorageMode() const;

    /**
     * @brief Returns the bounds of the image. This is the area
     * where the pixels are defined.
     **/
    const RectI& getBounds() const;

    /**
     * @brief Returns the scale of the image.
     **/
    const RenderScale& getProxyScale() const;

    /**
     * @brief Return the mipmap level of the image
     **/
    unsigned int getMipMapLevel() const;

    /**
     * @brief Converts the mipmap level to a scale factor
     **/
    static double getScaleFromMipMapLevel(unsigned int level);

    /**
     * @brief Converts the given scale factor to a mipmap level.
     * The scale factor must correspond to a mipmap level.
     **/
    static unsigned int getLevelFromScale(double s);

    /**
     * @brief Short for getLayer().getNumComponents()
     **/
    unsigned int getComponentsCount() const;

    /**
     * @brief Returns the layer that was passed to the create() function
     **/
    const ImageComponents& getLayer() const;

    /**
     * @brief Returns the bitdepth of the image as passed to the create() function
     **/
    ImageBitDepthEnum getBitDepth() const;

    /**
     * @brief This enum controls the behavior when doing the following conversion:
     * anything-->RGBA
     * anything-->Alpha
     **/
    enum AlphaChannelHandlingEnum
    {
        // If the alpha channel does not exist in input, it is filled with 0
        // otherwise it is copied from the input.
        eAlphaChannelHandlingCreateFill0,

        // If the alpha channel does not exist in input, it is filled with 1
        // otherwise it is copied from the input.
        eAlphaChannelHandlingCreateFill1,

        // The alpha channel is copied from the channel given by channelForAlpha
        eAlphaChannelHandlingFillFromChannel,

    };

    /**
     * @brief This enum controls the behavior when copying a single channel image
     * to a multiple channel image.
     **/
    enum MonoToPackedConversionEnum
    {
        // The mono channel will be copied to the channel given by conversionChannel
        // Other channels will be filled with a constant that is 0
        eMonoToPackedConversionCopyToChannelAndFillOthers,

        // The mono channel will be copied to the channel given by conversionChannel
        // Other channels will be leaved intact
        eMonoToPackedConversionCopyToChannelAndLeaveOthers,

        // The mono channel will be copied to all channels of the destination image
        eMonoToPackedConversionCopyToAll
    };

    struct CopyPixelsArgs
    {
        // The portion of rectangle to copy.
        //
        // Must be set to an area that intersects both image bounds.
        RectI roi;

        // A value in 0 <= index < 4 indicating the channel to use
        // for the AlphaChannelHandlingEnum and MonoToPackedConversionEnum flags
        //
        // Default - 0
        int conversionChannel;

        // @see AlphaChannelHandlingEnum
        //
        // Default - eAlphaChannelHandlingFillFromChannel
        AlphaChannelHandlingEnum alphaHandling;

        // @see MonoToPackedConversionEnum
        //
        // Default - eMonoToPackedConversionCopyToChannelAndLeaveOthers
        MonoToPackedConversionEnum monoConversion;

        // Input image is assumed to be in this colorspace
        // This is used when converting from bitdepth
        //
        // Default - eViewerColorSpaceLinear
        ViewerColorSpaceEnum srcColorspace;

        // Output image is assumed to be in this colorspace
        // This is used when converting from bitdepth
        //
        // Default - eViewerColorSpaceLinear
        ViewerColorSpaceEnum dstColorspace;

        // When converting from RGBA to RGB, if set to true the image
        // will be unpremultiplied.
        //
        // Default - false
        bool unPremultIfNeeded;

        // When copying the image, tiles that are marked cached on the
        // destination image will not be written to
        //
        // Default - false
        bool skipDestinationTilesMarkedCached;

        // If this image and the other image have the same number of components,
        // same buffer layout, same bitdepth and same bounds
        // by default the memory buffer pointers will be copied instead of all pixels.
        // This can be turned off by setting this flag to true
        //
        // Default - false
        bool forceCopyEvenIfBuffersHaveSameLayout;

        CopyPixelsArgs();
    };

    /**
     * @brief Copy pixels from another image in the given rectangle.
     * The other image may have a different bitdepth or components.
     *
     * If both images are tiled, their tile size may differ.
     * The other image will be copied to a temporary
     * untiled buffer which will then be copied to this image tiles.
     *
     * If the other image is an OpenGL texture and this image is tiled
     * the texture will be read into a temporary buffer first, to ensure
     * a single call to glReadPixels.
     *
     * @param conversionChannel A value in 0 <= index < 4 indicating the channel to use
     * for the AlphaChannelHandlingEnum and MonoToPackedConversionEnum flags
     **/
    void copyPixels(const Image& other, const CopyPixelsArgs& args);

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
    static void getABCDRectangles(const RectI& srcBounds, const RectI& biggerBounds, RectI& aRect, RectI& bRect, RectI& cRect, RectI& dRect);

    /**
     * @brief Helper function to get string from a layer and bitdepth
     **/
    static std::string getFormatString(const ImageComponents& comps, ImageBitDepthEnum depth);

    /**
     * @brief Helper function to get string from  bitdepth
     **/
    static std::string getDepthString(ImageBitDepthEnum depth);

    /**
     * @brief Helper function that returns true if the conversion of bitdepth is lossy
     **/
    static bool isBitDepthConversionLossy(ImageBitDepthEnum from, ImageBitDepthEnum to);

    /**
     * @brief Helper function to retrieve a pointer to the pixel at the given coordinates for an image with the given specs.
     **/
    static const unsigned char* pixelAtStatic(int x, int y, const RectI& bounds, int nComps, int dataSizeOf, const unsigned char* buf);
    static unsigned char* pixelAtStatic(int x, int y, const RectI& bounds, int nComps, int dataSizeOf, unsigned char* buf);

    /**
     * @brief Utility function to retrieve pointers to the RGBA buffers as well as the pixel stride (in the PIX type).
     * @param ptrs In input, if in a packed RGBA format, only the first pointer has been set and others are NULL. If co-planar
     * each pointer points to the appropriate channel or NULL if it does not exist.
     * @param dataSizeOf The number of bytes
     **/
    template <typename PIX, int nComps>
    static inline void getChannelPointers(const PIX* ptrs[4],
                                          int x, int y,
                                          const RectI& bounds,
                                          PIX* outPtrs[4],
                                          int* pixelStride)
    {
        const int dataSizeOf = sizeof(PIX);
        memset(outPtrs, 0, dataSizeOf * 4);
        {
            // If co-planar and number of components greater than 1, then ptrs[1] should be set,
            // In this case the pixel stride is always 1.
            // If nComps >1 and !ptrs[1] then we are in a packed format buffer.
            int redNComps = nComps;
            if (nComps != 1 && ptrs[1]) {
                redNComps = 1;
            }
            *pixelStride = redNComps;
            outPtrs[0] = (PIX*)pixelAtStatic(x, y, bounds, redNComps, dataSizeOf, (unsigned char*)ptrs[0]);
        }
        if (nComps > 1) {
            if (ptrs[1]) {
                outPtrs[1] = (PIX*)pixelAtStatic(x, y, bounds, 1, dataSizeOf, (unsigned char*)ptrs[1]);
            } else {
                outPtrs[1] = outPtrs[0] + 1;
            }
            if (nComps > 2) {
                if (ptrs[2]) {
                    outPtrs[2] = (PIX*)pixelAtStatic(x, y, bounds, 1, dataSizeOf, (unsigned char*)ptrs[2]);
                } else {
                    outPtrs[2] = outPtrs[1] + 1;
                }
            }
            if (nComps > 3) {
                if (ptrs[3]) {
                    outPtrs[3] = (PIX*)pixelAtStatic(x, y, bounds, 1, dataSizeOf, (unsigned char*)ptrs[3]);
                } else {
                    outPtrs[3] = outPtrs[2] + 1;
                }
            }
        }
    } // getChannelPointers

    template <typename PIX>
    static inline void getChannelPointers(const PIX* ptrs[4],
                                          int x, int y,
                                          const RectI& bounds,
                                          int nComps,
                                          PIX* outPtrs[4],
                                          int* pixelStride)
    {
        switch (nComps) {
            case 1:
                getChannelPointers<PIX, 1>(ptrs, x, y, bounds, outPtrs, pixelStride);
                break;
            case 2:
                getChannelPointers<PIX, 2>(ptrs, x, y, bounds, outPtrs, pixelStride);
                break;
            case 3:
                getChannelPointers<PIX, 3>(ptrs, x, y, bounds, outPtrs, pixelStride);
                break;
            case 4:
                getChannelPointers<PIX, 4>(ptrs, x, y, bounds, outPtrs, pixelStride);
                break;
            default:
                memset(outPtrs, 0, sizeof(void*) * 4);
                *pixelStride = 0;
                break;
        }

    }

    static void getChannelPointers(const void* ptrs[4],
                                   int x, int y,
                                   const RectI& bounds,
                                   int nComps,
                                   ImageBitDepthEnum bitdepth,
                                   void* outPtrs[4],
                                   int* pixelStride);


    struct CPUTileData
    {
        void* ptrs[4];
        RectI tileBounds;
        ImageBitDepthEnum bitDepth;
        int nComps;

        CPUTileData()
        : ptrs()
        , tileBounds()
        , bitDepth(eImageBitDepthNone)
        , nComps(0)
        {
            memset(ptrs, 0, sizeof(void*) * 4);
        }

        CPUTileData(const CPUTileData& other)
        : ptrs()
        , tileBounds(other.tileBounds)
        , bitDepth(other.bitDepth)
        , nComps(other.nComps)
        {
            memcpy(ptrs, other.ptrs, sizeof(void*) * 4);
        }
    };

    /**
     * @brief For an image that is represented as an OpenGL texture, returns the associated texture.
     **/
    GLCacheEntryPtr getGLCacheEntry() const;

    /**
     * @brief For a tile with CPU (RAM or MMAP) storage, returns the buffer data.
     **/
    void getCPUTileData(const Tile& tile, CPUTileData* data) const;

    static void getCPUTileData(const Tile& tile, ImageBufferLayoutEnum layout, CPUTileData* data);

    /**
     * @brief Returns the tile at the given tileIndex.
     * An untiled image has a single tile at index 0.
     **/
    bool getTileAt(int tileIndex, Tile* tile) const;

    /**
     * @brief Returns the number of tiles
     **/
    int getNumTiles() const;

    /**
     * @brief Returns the cache access policy for this image
     **/
    CacheAccessModeEnum getCachePolicy() const;

    /**
     * @brief If this image has cache write access, push the tiles to the cache.
     * This will only push the tiles that are marked eCacheEntryStatusMustCompute.
     * making them available to other threads waiting for it (and waking them up).
     **/
    void pushTilesToCacheIfNotAborted();

    /**
     * @brief If this image has some tiles pending (i.e: another thread is computing it),
     * wait for them to be available. This function returns once all tiles are marked as cached
     * or to be computed.
     * @returns true if everything is rendered, false if there's still tiles to render
     **/
    bool waitForPendingTiles();

    /**
     * @brief If this image is cached, returns whether other thread(s) are waiting for the resulting tiles or not.
     * If only this thread is interested on its tiles this function returns false.
     **/
    bool hasTilesPendingForOtherThreads() const;

    /**
     * @brief If this image is cached, this will return what portion of the image is left to render.
     * @param hasPendingResults[out] If set to true, then the caller should, after rendering the given rectangles
     * call waitForPendingTiles() and then afterwards recheck the rectangles left to render.
     **/
    std::list<RectI> getRestToRender(bool *hasPendingResults) const;


    /**
     * @brief Fills the image with the given colour. If the image components
     * are not RGBA it will ignore the unexisting components.
     * If the image is single channel, only the alpha value 'a' will
     * be used.
     * Filling the image with black and transparant is optimized
     **/
    void fill( const RectI & roi, float r, float g, float b, float a);

    /**
     * @brief Short for fill(roi, 0,0,0,0)
     **/
    void fillZero( const RectI& roi);

    /**
     * @brief Short for fill(getBounds(), 0,0,0,0)
     **/
    void fillBoundsZero();

    /**
     * @brief Ensures the image bounds can contain the given roi.
     * If it does not, a temporary image is created to the union of
     * the existing bounds and the passed RoI. 
     * Data is copied over to the temporary image and then swaped with this image.
     * In output of this function, the image bounds contain at least the RoI.
     **/
    void ensureBounds(const RectI& roi);


    /**
     * @brief Downscales by pow(2, downscaleLevels) a portion of this image and returns a new image with 
     * the downscaled data.
     * If downscaleLevels is 0, this will return this image.
     * The new image in output if created will have a packed RGBA full rect format.
     * If the roi will be rounded to the closest enclosing rectangle that has
     * a multiple of 2 width and height.
     **/
    ImagePtr downscaleMipMap(const RectI & roi, unsigned int downscaleLevels) const;

    /**
     * @brief Returns true if the image contains NaNs or infinite values, and fix them.
     * Currently, no OpenGL implementation is provided.
     */
    bool checkForNaNs(const RectI& roi) WARN_UNUSED_RETURN;


    /**
     * @brief Returns whether copyUnProcessedChannels() will have any effect at all
     **/
    bool canCallCopyUnProcessedChannels(std::bitset<4> processChannels) const;

    /**
     * @brief Given the channels to process, this function copies from the originalImage the channels
     * that are not marked to true in processChannels.
     **/
    void copyUnProcessedChannels(const RectI& roi,
                                 std::bitset<4> processChannels,
                                 const ImagePtr& originalImage);

    /**
     * @brief Mask the image by the given mask and also disolves it to the originalImg with the given mix.
     **/
    void applyMaskMix(const RectI& roi,
                      const ImagePtr& maskImg,
                      const ImagePtr& originalImg,
                      bool masked,
                      bool maskInvert,
                      float mix);


    /**
     * @brief Clamp the pixel to the given minval and maxval
     **/
    template <typename PIX>
    static PIX clamp(PIX x, PIX minval, PIX maxval);


    /**
     * @brief If PIX is an integer format (unsigned char/unsigned short)
     * then it is clamped between 0 and maxValue, otherwise if floating point type
     * it is left untouched.
     **/
    template<typename PIX>
    static PIX clampIfInt(float v);

    /**
     * @brief Convert pixel depth
     **/
    template <typename SRCPIX, typename DSTPIX>
    static DSTPIX convertPixelDepth(SRCPIX pix);


private:

    friend struct ImagePrivate;
    
    boost::scoped_ptr<ImagePrivate> _imp;
};

//template <> inline unsigned char clamp(unsigned char v) { return v; }
//template <> inline unsigned short clamp(unsigned short v) { return v; }
template <>
inline float
Image::clamp(float x,
             float minval,
             float maxval)
{
    return std::min(std::max(minval, x), maxval);
}

template <>
inline double
Image::clamp(double x,
             double minval,
             double maxval)
{
    return std::min(std::max(minval, x), maxval);
}

template<>
inline unsigned char
Image::clampIfInt(float v) { return (unsigned char)clamp<float>(v, 0, 255); }

template<>
inline unsigned short
Image::clampIfInt(float v) { return (unsigned short)clamp<float>(v, 0, 65535); }

template<>
inline float
Image::clampIfInt(float v) { return v; }

NATRON_NAMESPACE_EXIT;

#endif // NATRON_ENGINE_IMAGE_H
