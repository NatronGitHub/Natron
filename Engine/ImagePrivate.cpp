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

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "ImagePrivate.h"

#if !defined(SBK_RUN) && !defined(Q_MOC_RUN)
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_OFF
#include <boost/math/special_functions/fpclassify.hpp>
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_ON
#endif

#include <QDebug>
#include <QThread>

#include "Engine/Hash64.h"
#include "Engine/Node.h"

NATRON_NAMESPACE_ENTER

void
ImagePrivate::getCPUDataInternal(const RectI& bounds,
                                 int nComps,
                                 const ImageStorageBasePtr storage[4],
                                 ImageBitDepthEnum depth,
                                 ImageBufferLayoutEnum format,
                                 Image::CPUData* data)
{
    memset(data->ptrs, 0, sizeof(void*) * 4);
    data->bounds = bounds;
    data->bitDepth = depth;
    data->nComps = nComps;


    switch (format) {
        case eImageBufferLayoutRGBAPackedFullRect: {
            RAMImageStoragePtr fromIsRAMBuffer = toRAMImageStorage(storage[0]);
            assert(fromIsRAMBuffer);
            data->ptrs[0] = fromIsRAMBuffer->getData();
        }   break;
        case eImageBufferLayoutMonoChannelFullRect:
            for (int i = 0; i < data->nComps; ++i) {
                RAMImageStoragePtr fromIsRAMBuffer = toRAMImageStorage(storage[i]);
                assert(fromIsRAMBuffer);
                data->ptrs[i] = fromIsRAMBuffer->getData();
            }
            break;
        case eImageBufferLayoutRGBACoplanarFullRect: {
            RAMImageStoragePtr fromIsRAMBuffer = toRAMImageStorage(storage[0]);
            assert(fromIsRAMBuffer);
            data->ptrs[0] = fromIsRAMBuffer->getData();
            // Coplanar requires offsetting
            std::size_t planeSize = data->nComps * data->bounds.area() * getSizeOfForBitDepth(data->bitDepth);
            if (data->nComps > 1) {
                data->ptrs[1] = (char*)data->ptrs[0] + planeSize;
                if (data->nComps > 2) {
                    data->ptrs[2] = (char*)data->ptrs[1] + planeSize;
                    if (data->nComps > 3) {
                        data->ptrs[3] = (char*)data->ptrs[2] + planeSize;
                    }
                }
            }
        }   break;
    }
} // getCPUDataInternal

ActionRetCodeEnum
ImagePrivate::initAndFetchFromCache(const Image::InitStorageArgs& args)
{

    const std::string& planeID = plane.getPlaneID();

    // How many buffer should we make for a tile
    // A mono channel image should have one per channel
    std::vector<int> channelIndices;
    switch (bufferFormat) {
        case eImageBufferLayoutMonoChannelFullRect: {

            for (int nc = 0; nc < plane.getNumComponents(); ++nc) {
                channelIndices.push_back(nc);
            }
        }   break;
        case eImageBufferLayoutRGBACoplanarFullRect:
        case eImageBufferLayoutRGBAPackedFullRect:
            channelIndices.push_back(-1);
            break;
    }


    EffectInstancePtr effect = renderClone.lock();
    std::string pluginID;
    if (effect) {
        pluginID = effect->getNode()->getPluginID();
    }


    // Create storage for each channels
    try {
        for (std::size_t c = 0; c < channelIndices.size(); ++c) {
            boost::shared_ptr<AllocateMemoryArgs> allocArgs;
            // Allocate a new entry
            switch (storage) {
                case eStorageModeGLTex: {
                    GLImageStoragePtr buffer(new GLImageStorage());
                    channels[c] = buffer;
                    boost::shared_ptr<GLAllocateMemoryArgs> a(new GLAllocateMemoryArgs());
                    a->textureTarget = args.textureTarget;
                    a->glContext = args.glContext;
                    a->bounds = originalBounds;
                    a->bitDepth = bitdepth;
                    allocArgs = a;
                }   break;
                case eStorageModeRAM: {
                    RAMImageStoragePtr buffer(new RAMImageStorage());
                    channels[c] = buffer;
                    boost::shared_ptr<RAMAllocateMemoryArgs> a(new RAMAllocateMemoryArgs());
                    a->bitDepth = bitdepth;
                    a->bounds = originalBounds;

                    if (channelIndices[c] == -1) {
                        a->numComponents = (std::size_t)plane.getNumComponents();
                    } else {
                        a->numComponents = 1;
                    }
                    allocArgs = a;
                }   break;
                case eStorageModeNone:
                    return eActionStatusFailed;
            }
            assert(allocArgs && channels[c]);

            if (tilesAllocated) {
                // Allocate the memory for the tile.
                // This may throw a std::bad_alloc
                channels[c]->allocateMemory(*allocArgs);
            } else {
                // Delay the allocation
                channels[c]->setAllocateMemoryArgs(allocArgs);
            }

        }
    } catch (const std::bad_alloc&) {
        return eActionStatusFailed;
    }

    if (cachePolicy == eCacheAccessModeNone && !args.createTilesMapEvenIfNoCaching) {
        return eActionStatusOK;
    }
    assert(cachePolicy == eCacheAccessModeReadWrite || cachePolicy == eCacheAccessModeWriteOnly || args.createTilesMapEvenIfNoCaching);

    // Make a hash value for the channel
    U64 layerID;
    {
        Hash64 hash;
        Hash64::appendQString(QString::fromUtf8(planeID.c_str()), &hash);
        hash.computeHash();
        layerID = hash.value();
    }

    ImageCacheKeyPtr key(new ImageCacheKey(args.nodeTimeViewVariantHash,
                                           layerID,
                                           args.proxyScale,
                                           pluginID));


    cacheEntry.reset(new ImageCacheEntry(_publicInterface->shared_from_this(),
                                         args.perMipMapPixelRoD,
                                         args.bounds,
                                         args.isDraft,
                                         args.mipMapLevel,
                                         args.bitdepth,
                                         plane.getNumComponents(),
                                         channels,
                                         args.bufferFormat,
                                         args.renderClone,
                                         key,
                                         cachePolicy));

    return eActionStatusOK;
} // initTileAndFetchFromCache



ActionRetCodeEnum
ImagePrivate::initFromExternalBuffer(const Image::InitStorageArgs& args)
{
    assert(args.externalBuffer);

    if (args.bitdepth != args.externalBuffer->getBitDepth()) {
        assert(false);
        // When providing an external buffer, the bitdepth must be the same as the requested depth
        return eActionStatusFailed;
    }

    GLImageStoragePtr isGLBuffer = toGLImageStorage(args.externalBuffer);
    RAMImageStoragePtr isRAMBuffer = toRAMImageStorage(args.externalBuffer);
    if (isGLBuffer) {
        if (args.storage != eStorageModeGLTex) {
            return eActionStatusFailed;
        }
        if (isGLBuffer->getBounds() != args.bounds) {
            return eActionStatusFailed;
        }
        channels[0] = isGLBuffer;
    } else if (isRAMBuffer) {
        if (args.storage != eStorageModeRAM) {
            return eActionStatusFailed;
        }
        if (isRAMBuffer->getBounds() != args.bounds) {
            return eActionStatusFailed;
        }
        if (isRAMBuffer->getNumComponents() != (std::size_t)args.plane.getNumComponents()) {
            return eActionStatusFailed;
        }
        channels[0] = isRAMBuffer;
    } else {
        // Unrecognized storage
        return eActionStatusFailed;
    }
    return eActionStatusOK;
} // initFromExternalBuffer

/**
 * @brief If copying pixels from fromImage to toImage cannot be copied directly, this function
 * returns a temporary image that is suitable to copy then to the toImage.
 **/
ActionRetCodeEnum
ImagePrivate::checkIfCopyToTempImageIsNeeded(const Image& fromImage, const Image& toImage, const RectI& roi, ImagePtr* outImage)
{
    // OpenGL textures may only be read from a RGBA packed buffer
    if (fromImage.getStorageMode() == eStorageModeGLTex) {

        // If this is also an OpenGL texture, check they have the same context otherwise first bring back
        // the image to CPU
        if (toImage.getStorageMode() == eStorageModeGLTex) {

            GLImageStoragePtr isGlEntry = toGLImageStorage(toImage._imp->channels[0]);
            GLImageStoragePtr otherIsGlEntry = toGLImageStorage(fromImage._imp->channels[0]);
            assert(isGlEntry && otherIsGlEntry);
            if (isGlEntry->getOpenGLContext() != otherIsGlEntry->getOpenGLContext()) {
                ImagePtr tmpImage;
                Image::InitStorageArgs args;
                args.renderClone = fromImage._imp->renderClone.lock();
                args.bounds = fromImage.getBounds();
                args.mipMapLevel = fromImage.getMipMapLevel();
                args.proxyScale = fromImage.getProxyScale();
                args.plane = ImagePlaneDesc::getRGBAComponents();
                tmpImage = Image::create(args);
                if (!tmpImage) {
                    return eActionStatusFailed;
                }
                Image::CopyPixelsArgs copyArgs;
                copyArgs.roi = roi;
                ActionRetCodeEnum stat = tmpImage->copyPixels(fromImage, copyArgs);
                if (isFailureRetCode(stat)) {
                    return stat;
                }
                *outImage = tmpImage;
                return eActionStatusOK;
            }
        } else if (toImage._imp->bufferFormat != eImageBufferLayoutRGBAPackedFullRect || (toImage.getComponentsCount() != 4) || toImage.getBounds() != fromImage.getBounds()) {
            // Converting from OpenGL to CPU requires a RGBA buffer with the same bounds
            ImagePtr tmpImage;
            Image::InitStorageArgs args;
            args.renderClone = fromImage._imp->renderClone.lock();
            args.bounds = fromImage.getBounds();
            args.mipMapLevel = fromImage.getMipMapLevel();
            args.proxyScale = fromImage.getProxyScale();
            args.plane = ImagePlaneDesc::getRGBAComponents();
            tmpImage = Image::create(args);
            if (!tmpImage) {
                return eActionStatusFailed;
            }
            Image::CopyPixelsArgs copyArgs;

            // Copy the full bounds since we are converting the OpenGL texture to RAM (we have no other choice since glReadPixels
            // expect a buffer with the appropriate size)
            copyArgs.roi = args.bounds;
            copyArgs.alphaHandling = Image::eAlphaChannelHandlingCreateFill1;
            ActionRetCodeEnum stat = tmpImage->copyPixels(fromImage, copyArgs);
            if (isFailureRetCode(stat)) {
                return stat;
            }
            *outImage = tmpImage;
            return eActionStatusOK;

        }

        // All other cases can copy fine
        return eActionStatusOK;
    }

    // OpenGL textures may only be written from a RGBA packed buffer
    if (toImage.getStorageMode() == eStorageModeGLTex) {

        // Converting to OpenGl requires an RGBA buffer
        if (fromImage._imp->bufferFormat != eImageBufferLayoutRGBAPackedFullRect || fromImage.getComponentsCount() != 4) {
            ImagePtr tmpImage;
            Image::InitStorageArgs args;
            args.renderClone = fromImage._imp->renderClone.lock();
            args.bounds = fromImage.getBounds();
            args.plane = ImagePlaneDesc::getRGBAComponents();
            args.mipMapLevel = fromImage.getMipMapLevel();
            args.proxyScale = fromImage.getProxyScale();
            tmpImage = Image::create(args);
            if (!tmpImage) {
                return eActionStatusFailed;
            }
            Image::CopyPixelsArgs copyArgs;
            copyArgs.roi = roi;
            copyArgs.alphaHandling = Image::eAlphaChannelHandlingCreateFill1;
            ActionRetCodeEnum stat = tmpImage->copyPixels(fromImage, copyArgs);
            if (isFailureRetCode(stat)) {
                return stat;
            }
            *outImage = tmpImage;
            return eActionStatusOK;
        }
    }

    // All other cases can copy fine
    return eActionStatusOK;
} // checkIfCopyToTempImageIsNeeded

template <typename PIX, int maxValue, int nComps>
static ActionRetCodeEnum
halveImageForInternal(const void* srcPtrs[4],
                      const RectI& srcBounds,
                      void* dstPtrs[4],
                      const RectI& dstBounds,
                      const EffectInstancePtr& renderClone)
{

    PIX* dstPixelPtrs[4] = {NULL, NULL, NULL, NULL};
    int dstPixelStride;
    Image::getChannelPointers<PIX, nComps>((const PIX**)dstPtrs, dstBounds.x1, dstBounds.y1, dstBounds, (PIX**)dstPixelPtrs, &dstPixelStride);

    const PIX* srcPixelPtrs[4] = {NULL, NULL, NULL, NULL};
    int srcPixelStride;
    Image::getChannelPointers<PIX, nComps>((const PIX**)srcPtrs, srcBounds.x1, srcBounds.y1, srcBounds, (PIX**)srcPixelPtrs, &srcPixelStride);

    const int dstRowElementsCount = dstBounds.width() * dstPixelStride;
    const int srcRowElementsCount = srcBounds.width() * srcPixelStride;


    for (int y = dstBounds.y1; y < dstBounds.y2; ++y) {

        if (renderClone && renderClone->isRenderAborted()) {
            return eActionStatusAborted;
        }

        // The current dst row, at y, covers the src rows y*2 (thisRow) and y*2+1 (nextRow).
        const int srcy = y * 2;

        // Check that we are within srcBounds.
        const bool pickThisRow = srcBounds.y1 <= (srcy + 0) && (srcy + 0) < srcBounds.y2;
        const bool pickNextRow = srcBounds.y1 <= (srcy + 1) && (srcy + 1) < srcBounds.y2;

        const int sumH = (int)pickNextRow + (int)pickThisRow;
        assert(sumH == 1 || sumH == 2);

        for (int x = dstBounds.x1; x < dstBounds.x2; ++x) {

            // The current dst col, at y, covers the src cols x*2 (thisCol) and x*2+1 (nextCol).
            const int srcx = x * 2;

            // Check that we are within srcBounds.
            const bool pickThisCol = srcBounds.x1 <= (srcx + 0) && (srcx + 0) < srcBounds.x2;
            const bool pickNextCol = srcBounds.x1 <= (srcx + 1) && (srcx + 1) < srcBounds.x2;

            const int sumW = (int)pickThisCol + (int)pickNextCol;
            assert(sumW == 1 || sumW == 2);

            const int sum = sumW * sumH;
            assert(0 < sum && sum <= 4);

            for (int k = 0; k < nComps; ++k) {

                // Averaged pixels are as such:
                // a b
                // c d

                const PIX a = (pickThisCol && pickThisRow) ? *(srcPixelPtrs[k]) : 0;
                const PIX b = (pickNextCol && pickThisRow) ? *(srcPixelPtrs[k] + srcPixelStride) : 0;
                const PIX c = (pickThisCol && pickNextRow) ? *(srcPixelPtrs[k] + srcRowElementsCount) : 0;
                const PIX d = (pickNextCol && pickNextRow) ? *(srcPixelPtrs[k] + srcRowElementsCount + srcPixelStride)  : 0;

                assert( sumW == 2 || ( sumW == 1 && ( (a == 0 && c == 0) || (b == 0 && d == 0) ) ) );
                assert( sumH == 2 || ( sumH == 1 && ( (a == 0 && b == 0) || (c == 0 && d == 0) ) ) );

                *dstPixelPtrs[k] = (a + b + c + d) / sum;

                srcPixelPtrs[k] += srcPixelStride * 2;
                dstPixelPtrs[k] += dstPixelStride;
            } // for each component

        } // for each pixels on the line

        // Remove what was offset to the pointers during this scan-line and offset to the next
        for (int k = 0; k < nComps; ++k) {
            dstPixelPtrs[k] += (dstRowElementsCount - dstBounds.width() * dstPixelStride);
            srcPixelPtrs[k] += ((srcRowElementsCount - dstBounds.width() * srcPixelStride) * 2);
        }
    }  // for each scan line
    return eActionStatusOK;
} // halveImageForInternal


template <typename PIX, int maxValue>
static ActionRetCodeEnum
halveImageForDepth(const void* srcPtrs[4],
                   int nComps,
                   const RectI& srcBounds,
                   void* dstPtrs[4],
                   const RectI& dstBounds,
                   const EffectInstancePtr& renderClone)
{
    switch (nComps) {
        case 1:
            return halveImageForInternal<PIX, maxValue, 1>(srcPtrs, srcBounds, dstPtrs, dstBounds, renderClone);
        case 2:
            return halveImageForInternal<PIX, maxValue, 2>(srcPtrs, srcBounds, dstPtrs, dstBounds, renderClone);
        case 3:
            return halveImageForInternal<PIX, maxValue, 3>(srcPtrs, srcBounds, dstPtrs, dstBounds, renderClone);
        case 4:
            return halveImageForInternal<PIX, maxValue, 4>(srcPtrs, srcBounds, dstPtrs, dstBounds, renderClone);
        default:
        return eActionStatusFailed;
    }
}


ActionRetCodeEnum
ImagePrivate::halveImage(const void* srcPtrs[4],
                         int nComps,
                         ImageBitDepthEnum bitDepth,
                         const RectI& srcBounds,
                         void* dstPtrs[4],
                         const RectI& dstBounds,
                         const EffectInstancePtr& renderClone)
{
    switch ( bitDepth ) {
        case eImageBitDepthByte:
            return halveImageForDepth<unsigned char, 255>(srcPtrs, nComps, srcBounds, dstPtrs, dstBounds, renderClone);
        case eImageBitDepthShort:
            return halveImageForDepth<unsigned short, 65535>(srcPtrs, nComps, srcBounds, dstPtrs, dstBounds, renderClone);
        case eImageBitDepthFloat:
            return halveImageForDepth<float, 1>(srcPtrs, nComps, srcBounds, dstPtrs, dstBounds, renderClone);
            break;
        default:
        return eActionStatusFailed;
    }
} // halveImage


template <typename PIX, int maxValue, int nComps>
ActionRetCodeEnum
checkForNaNsInternal(void* ptrs[4],
                     const RectI& bounds,
                     const RectI& roi,
                     const EffectInstancePtr& effect,
                     bool* foundNan)
{
    *foundNan = false;

    PIX* dstPixelPtrs[4] = {NULL, NULL, NULL, NULL};
    int dstPixelStride;
    Image::getChannelPointers<PIX, nComps>((const PIX**)ptrs, roi.x1, roi.y1, bounds, (PIX**)dstPixelPtrs, &dstPixelStride);
    const int rowElementsCount = bounds.width() * dstPixelStride;

    for (int y = roi.y1; y < roi.y2; ++y) {
        if (effect && effect->isRenderAborted()) {
            return eActionStatusAborted;
        }
        for (int x = roi.x1; x < roi.x2; ++x) {
            for (int k = 0; k < nComps; ++k) {
                // we remove NaNs, but infinity values should pose no problem
                // (if they do, please explain here which ones)
                if ( (boost::math::isnan)(*dstPixelPtrs[k]) ) { // check for NaN
                    *dstPixelPtrs[k] = 1.;
                    *foundNan = true;
                }
                dstPixelPtrs[k] += dstPixelStride;
            }
        }
        // Remove what was done at the previous scan-line and got to the next
        for (int k = 0; k < nComps; ++k) {
            dstPixelPtrs[k] += (rowElementsCount - roi.width() * dstPixelStride);
        }
    } // for each scan-line
    return eActionStatusOK;
} // checkForNaNsInternal

template <typename PIX, int maxValue>
ActionRetCodeEnum
checkForNaNsForDepth(void* ptrs[4],
                     int nComps,
                     const RectI& bounds,
                     const RectI& roi,
                     const EffectInstancePtr& effect,
                     bool* foundNan)
{
    switch (nComps) {
        case 1:
            return checkForNaNsInternal<PIX, maxValue, 1>(ptrs, bounds, roi, effect, foundNan);
        case 2:
            return checkForNaNsInternal<PIX, maxValue, 2>(ptrs, bounds, roi, effect, foundNan);
        case 3:
            return checkForNaNsInternal<PIX, maxValue, 3>(ptrs, bounds, roi, effect, foundNan);
        case 4:
            return checkForNaNsInternal<PIX, maxValue, 4>(ptrs, bounds, roi, effect, foundNan);
        default:
            break;
    }
    return eActionStatusFailed;

}


ActionRetCodeEnum
ImagePrivate::checkForNaNs(void* ptrs[4],
                           int nComps,
                           ImageBitDepthEnum bitdepth,
                           const RectI& bounds,
                           const RectI& roi,
                           const EffectInstancePtr& effect,
                           bool* foundNan)
{
    switch ( bitdepth ) {
        case eImageBitDepthByte:
            return checkForNaNsForDepth<unsigned char, 255>(ptrs, nComps, bounds, roi, effect, foundNan);
        case eImageBitDepthShort:
            return checkForNaNsForDepth<unsigned short, 65535>(ptrs, nComps, bounds, roi, effect, foundNan);
        case eImageBitDepthHalf:
            assert(false);
            return eActionStatusFailed;
        case eImageBitDepthFloat:
            return checkForNaNsForDepth<float, 1>(ptrs, nComps, bounds, roi, effect, foundNan);
        case eImageBitDepthNone:
            return eActionStatusFailed;
    }
    return eActionStatusFailed;
}

NATRON_NAMESPACE_EXIT
