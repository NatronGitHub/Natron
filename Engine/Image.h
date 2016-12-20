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

#ifndef NATRON_ENGINE_IMAGE_H
#define NATRON_ENGINE_IMAGE_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include <bitset>

#include "Global/Macros.h"

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/scoped_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#endif

#include "Global/GlobalDefines.h"

#include "Engine/BufferableObject.h"
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
: public BufferableObject
, public boost::enable_shared_from_this<Image>
{
    /**
     * @brief Ctor. Make a new empty image. This is private, the static create() function should be used instead.
     **/
    Image();

public:
    
    enum CacheAccessModeEnum
    {
        // The image should not use the cache at all
        eCacheAccessModeNone,
        
        // The image shoud try to look for a match in the cache
        // if possible. Tiles that are allocated or modified will be
        // pushed to the cache.
        eCacheAccessModeReadWrite,
        
        // The image should use cached tiles but can write to the cache
        eCacheAccessModeWriteOnly
    };

    enum ImageBufferLayoutEnum
    {
        // This will make an image with an internal storage composed
        // of one or multiple tiles, each of which is a single channel buffer.
        // This is the preferred storage format for the Cache.
        eImageBufferLayoutMonoChannelTiled,


        // This will make an image with an internal storage composed of a single
        // buffer for all r g b and a channels. The buffer is layout as such:
        // all red pixels first, then all greens, then blue then alpha:
        // RRRRRRGGGGGBBBBBAAAAA
        eImageBufferLayoutRGBACoplanarFullRect,

        // This will make an image with an internal storage composed of a single
        // packed RGBA buffer. Pixels layout is as such: RGBARGBARGBA
        // This is the preferred layout by default for OpenFX.
        // OpenGL textures only support this mode for now.
        eImageBufferLayoutRGBAPackedFullRect,
    };

    struct InitStorageArgs
    {
        // The bounds of the image. This will internally set the required number of tiles
        //
        // Must be set
        RectI bounds;

        // A pointer to another image that should be updated when this image dies.
        // Since tiled mono-channel format may not be ideal for most processing formats
        // you may provide a pointer to another mirror image with another format.
        // When this image is destroyed, it will update the mirror image.
        // If the mirror image is cached, it will in turn update the cache when destroyed.
        // The mirror image should may have different bitdepth, different
        // storage and components.
        ImagePtr mirrorImage;

        // If mirrorImage is set, this is the portion to update in the mirror image.
        // This should be contained in mirrorImage
        RectI mirrorImageRoI;

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

        // Indicates the desired buffer format for the image. This defaults to eImageBufferLayoutRGBAPackedFullRect
        //
        // Default - eImageBufferLayoutRGBAPackedFullRect
        ImageBufferLayoutEnum bufferFormat;
        
        // The mipmaplevel of the image
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
     * @brief Returns the internal storage of the image that was supplied in the create() function.
     **/
    StorageModeEnum getStorageMode() const;

    /**
     * @brief Returns the bounds of the image. This is the area
     * where the pixels are defined.
     **/
    RectI getBounds() const;

    /**
     * @brief Returns the mip map level of the image.
     **/
    unsigned int getMipMapLevel() const;

    /**
     * @brief Short for getComponents().getNumComponents()
     **/
    unsigned int getComponentsCount() const;

    /**
     * @brief Returns the layer that was passed to the create() function
     **/
    const ImageComponents& getComponents() const;

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

    template <typename GL>
    static void setupGLViewport(const RectI& bounds, const RectI& roi)
    {
        GL::Viewport( roi.x1 - bounds.x1, roi.y1 - bounds.y1, roi.width(), roi.height() );
        glCheckError(GL);
        GL::MatrixMode(GL_PROJECTION);
        GL::LoadIdentity();
        GL::Ortho( roi.x1, roi.x2,
                    roi.y1, roi.y2,
                    -10.0 * (roi.y2 - roi.y1), 10.0 * (roi.y2 - roi.y1) );
        glCheckError(GL);
        GL::MatrixMode(GL_MODELVIEW);
        GL::LoadIdentity();
    }

    template <typename GL>
    static void applyTextureMapping(const RectI& srcBounds, const RectI& dstBounds, const RectI& roi)
    {
        setupGLViewport<GL>(dstBounds, roi);

        // Compute the texture coordinates to match the srcRoi
        Point srcTexCoords[4], vertexCoords[4];
        vertexCoords[0].x = roi.x1;
        vertexCoords[0].y = roi.y1;
        srcTexCoords[0].x = (roi.x1 - srcBounds.x1) / (double)srcBounds.width();
        srcTexCoords[0].y = (roi.y1 - srcBounds.y1) / (double)srcBounds.height();

        vertexCoords[1].x = roi.x2;
        vertexCoords[1].y = roi.y1;
        srcTexCoords[1].x = (roi.x2 - srcBounds.x1) / (double)srcBounds.width();
        srcTexCoords[1].y = (roi.y1 - srcBounds.y1) / (double)srcBounds.height();

        vertexCoords[2].x = roi.x2;
        vertexCoords[2].y = roi.y2;
        srcTexCoords[2].x = (roi.x2 - srcBounds.x1) / (double)srcBounds.width();
        srcTexCoords[2].y = (roi.y2 - srcBounds.y1) / (double)srcBounds.height();

        vertexCoords[3].x = roi.x1;
        vertexCoords[3].y = roi.y2;
        srcTexCoords[3].x = (roi.x1 - srcBounds.x1) / (double)srcBounds.width();
        srcTexCoords[3].y = (roi.y2 - srcBounds.y1) / (double)srcBounds.height();

        GL::Begin(GL_POLYGON);
        for (int i = 0; i < 4; ++i) {
            GL::TexCoord2d(srcTexCoords[i].x, srcTexCoords[i].y);
            GL::Vertex2d(vertexCoords[i].x, vertexCoords[i].y);
        }
        GL::End();
        glCheckError(GL);
    }

private:

    static void resizeInternal(const OSGLContextPtr& glContext,
                               const Image* srcImg,
                               const RectI& srcBounds,
                               const RectI& merge,
                               bool fillWithBlackAndTransparent,
                               bool setBitmapTo1,
                               bool createInCache,
                               ImagePtr* outputImage);

public:




    static std::string getFormatString(const ImageComponents& comps, ImageBitDepthEnum depth);
    static std::string getDepthString(ImageBitDepthEnum depth);
    static bool isBitDepthConversionLossy(ImageBitDepthEnum from, ImageBitDepthEnum to);




    static const unsigned char* pixelAtStatic(int x, int y, const RectI& bounds, int nComps, int dataSizeOf, const unsigned char* buf);

    static unsigned char* pixelAtStatic(int x, int y, const RectI& bounds, int nComps, int dataSizeOf, unsigned char* buf);

    static inline unsigned char* getPixelAddress_internal(int x, int y, unsigned char* basePtr, int pixelSize, const RectI& bounds)
    {
        return basePtr + (qint64)( y - bounds.y1 ) * pixelSize * bounds.width() + (qint64)( x - bounds.x1 ) * pixelSize;
    }

private:

    /**
     * @brief Access pixels. The pointer must be cast to the appropriate type afterwards.
     **/
    unsigned char* pixelAt(int x, int y);
    const unsigned char* pixelAt(int x, int y) const;


public:

    /**
     * @brief Fills the image with the given colour. If the image components
     * are not RGBA it will ignore the unsupported components.
     * For example if the image comps is eImageComponentAlpha, then only the alpha value 'a' will
     * be used.
     **/
    void fill( const RectI & roi, float r, float g, float b, float a, const OSGLContextPtr& glContext = OSGLContextPtr() );

    void fillZero( const RectI& roi, const OSGLContextPtr& glContext = OSGLContextPtr() );

    void fillBoundsZero( const OSGLContextPtr& glContext = OSGLContextPtr() );

    /**
     * @brief Same as fill(const RectI&,float,float,float,float) but fills the R,G and B
     * components with the same value.
     **/
    void fill( const RectI & rect,
               float colorValue = 0.f,
               float alphaValue = 1.f,
               const OSGLContextPtr& glContext = OSGLContextPtr() )
    {
        fill(rect, colorValue, colorValue, colorValue, alphaValue, glContext);
    }


    /**
     * @brief Downscales a portion of this image into output.
     * This function will adjust roi to the largest enclosed rectangle for the
     * given mipmap level,
     * and then computes the mipmap of the given level of that rectangle.
     **/
    void downscaleMipMap(const RectD& rod,
                         const RectI & roi,
                         unsigned int fromLevel, unsigned int toLevel,
                         bool copyBitMap,
                         Image* output) const;

    /**
     * @brief Upscales a portion of this image into output.
     * If the upscaled roi does not fit into output's bounds, it is cropped first.
     **/
    void upscaleMipMap(const RectI & roi, unsigned int fromLevel, unsigned int toLevel, Image* output) const;


    static double getScaleFromMipMapLevel(unsigned int level);
    static unsigned int getLevelFromScale(double s);


    template <typename PIX, bool doPremult>
    void premultInternal(const RectI& roi);
    template <bool doPremult>
    void premultForDepth(const RectI& roi);

public:

    /**
     * @brief Premultiply the image by its alpha channel on the given RoI.
     * Currently there is no implementation for OpenGL textures.
     **/
    void premultImage(const RectI& roi);

    /**
     * @brief Unpremultiply the image by its alpha channel on the given RoI.
     * Currently there is no implementation for OpenGL textures.
     **/
    void unpremultImage(const RectI& roi);

    bool canCallCopyUnProcessedChannels(std::bitset<4> processChannels) const;

    /**
     * @brief Given the channels to process, this function copies from the originalImage the channels
     * that are not marked to true in processChannels.
     **/
    void copyUnProcessedChannels( const RectI& roi,
                                  ImagePremultiplicationEnum outputPremult,
                                  ImagePremultiplicationEnum originalImagePremult,
                                  std::bitset<4> processChannels,
                                  const ImagePtr& originalImage,
                                  bool ignorePremult,
                                  const OSGLContextPtr& glContext = OSGLContextPtr() );

    /**
     * @brief Mask the image by the given mask and also disolves it to the originalImg with the given mix.
     **/
    void applyMaskMix( const RectI& roi,
                       const Image* maskImg,
                       const Image* originalImg,
                       bool masked,
                       bool maskInvert,
                       float mix,
                       const OSGLContextPtr& glContext = OSGLContextPtr() );

    /**
     * @brief Eeturns true if image contains NaNs or infinite values, and fix them.
     * Currently, no OpenGL implementation is provided.
     */
    bool checkForNaNs(const RectI& roi) WARN_UNUSED_RETURN;

    void copyBitmapRowPortion(int x1, int x2, int y, const Image& other);

    void copyBitmapPortion(const RectI& roi, const Image& other);

    template <typename PIX>
    static PIX clamp(PIX x, PIX minval, PIX maxval);


    template<typename PIX>
    static PIX clampIfInt(float v);

    template <typename SRCPIX, typename DSTPIX>
    static DSTPIX convertPixelDepth(SRCPIX pix);

private:

    template<int srcNComps, int dstNComps, typename PIX, int maxValue, bool masked, bool maskInvert>
    void applyMaskMixForMaskInvert(const RectI& roi,
                                   const Image* maskImg,
                                   const Image* originalImg,
                                   float mix);


    template<int srcNComps, int dstNComps, typename PIX, int maxValue, bool masked>
    void applyMaskMixForMasked(const RectI& roi,
                               const Image* maskImg,
                               const Image* originalImg,
                               bool maskInvert,
                               float mix);

    template<int srcNComps, int dstNComps, typename PIX, int maxValue>
    void applyMaskMixForDepth(const RectI& roi,
                              const Image* maskImg,
                              const Image* originalImg,
                              bool masked,
                              bool maskInvert,
                              float mix);


    template<int srcNComps, int dstNComps>
    void applyMaskMixForDstComponents(const RectI& roi,
                                      const Image* maskImg,
                                      const Image* originalImg,
                                      bool masked,
                                      bool maskInvert,
                                      float mix);

    template<int srcNComps>
    void applyMaskMixForSrcComponents(const RectI& roi,
                                      const Image* maskImg,
                                      const Image* originalImg,
                                      bool masked,
                                      bool maskInvert,
                                      float mix);

    template <typename PIX, int maxValue, int srcNComps, int dstNComps, bool doR, bool doG, bool doB, bool doA, bool premult, bool originalPremult, bool ignorePremult>
    void copyUnProcessedChannelsForPremult(std::bitset<4> processChannels,
                                           const RectI& roi,
                                           const ImagePtr& originalImage);

    template <typename PIX, int maxValue, int srcNComps, int dstNComps, bool ignorePremult>
    void copyUnProcessedChannelsForPremult(bool premult, bool originalPremult,
                                           std::bitset<4> processChannels,
                                           const RectI& roi,
                                           const ImagePtr& originalImage);

    template <typename PIX, int maxValue, int srcNComps, int dstNComps, bool doR, bool doG, bool doB, bool doA>
    void copyUnProcessedChannelsForChannels(const std::bitset<4> processChannels,
                                            const bool premult,
                                            const RectI& roi,
                                            const ImagePtr& originalImage,
                                            const bool originalPremult,
                                            const bool ignorePremult);

    template <typename PIX, int maxValue, int srcNComps, int dstNComps>
    void copyUnProcessedChannelsForChannels(const std::bitset<4> processChannels,
                                            const bool premult,
                                            const RectI& roi,
                                            const ImagePtr& originalImage,
                                            const bool originalPremult,
                                            const bool ignorePremult);


    template <typename PIX, int maxValue, int srcNComps, int dstNComps>
    void copyUnProcessedChannelsForComponents(bool premult,
                                              const RectI& roi,
                                              const std::bitset<4> processChannels,
                                              const ImagePtr& originalImage,
                                              const bool originalPremult,
                                              const bool ignorePremult);

    template <typename PIX, int maxValue>
    void copyUnProcessedChannelsForDepth(bool premult,
                                         const RectI& roi,
                                         std::bitset<4> processChannels,
                                         const ImagePtr& originalImage,
                                         bool originalPremult,
                                         bool ignorePremult);


    /**
     * @brief Given the output buffer,the region of interest and the mip map level, this
     * function computes the mip map of this image in the given roi.
     * If roi is NOT a power of 2, then it will be rounded to the closest power of 2.
     **/
    void buildMipMapLevel(const RectD& dstRoD, const RectI & roiCanonical, unsigned int level, bool copyBitMap,
                          Image* output) const;


    /**
     * @brief Halve the given roi of this image into output.
     * If the RoI bounds are odd, the largest enclosing RoI with even bounds will be considered.
     **/
    void halveRoI(const RectI & roi, bool copyBitMap,
                  Image* output) const;


    template <typename PIX, int maxValue>
    void halveRoIForDepth(const RectI & roi,
                          bool copyBitMap,
                          Image* output) const;

    /**
     * @brief Same as halveRoI but for 1D only (either width == 1 or height == 1)
     **/
    void halve1DImage(const RectI & roi, Image* output) const;

    template <typename PIX, int maxValue>
    void halve1DImageForDepth(const RectI & roi, Image* output) const;

    template <typename PIX, int maxValue>
    void upscaleMipMapForDepth(const RectI & roi, unsigned int fromLevel, unsigned int toLevel, Image* output) const;

    template <typename PIX, int maxValue>
    void fillForDepth(const RectI & roi, float r, float g, float b, float a);

    template <typename PIX, int maxValue, int nComps>
    void fillForDepthForComponents(const RectI & roi_,  float r, float g, float b, float a);

    template<typename PIX>
    void scaleBoxForDepth(const RectI & roi, Image* output) const;

private:

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
