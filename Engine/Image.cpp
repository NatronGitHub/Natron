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

#include "Image.h"

#include <algorithm> // min, max
#include <cassert>
#include <cstring> // for std::memcpy, std::memset
#include <stdexcept>

#if !defined(SBK_RUN) && !defined(Q_MOC_RUN)
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_OFF
#include <boost/math/special_functions/fpclassify.hpp>
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_ON
#endif

#include <QtCore/QDebug>
#include <QtCore/QThread>

#include "Engine/ImagePrivate.h"


#ifndef M_LN2
#define M_LN2       0.693147180559945309417232121458176568  /* loge(2)        */
#endif

// When defined, tiles will be fetched from the cache (and optionnally downscaled) sequentially
//#define NATRON_IMAGE_SEQUENTIAL_INIT

NATRON_NAMESPACE_ENTER

Image::Image()
: _imp(new ImagePrivate(this))
{

}

ImagePtr
Image::create(const InitStorageArgs& args)
{
    ImagePtr ret(new Image);
    ActionRetCodeEnum stat = ret->_imp->init(args);
    if (isFailureRetCode(stat)) {
        return ImagePtr();
    }
    return ret;
}

Image::~Image()
{


    // If this image is the last image holding a pointer to memory buffers, ensure these buffers
    // gets deallocated in a specific thread and not a render thread
    std::list<ImageStorageBasePtr> toDeleteInDeleterThread;
    for (int i = 0; i < 4; ++i) {
        if (_imp->channels[i]) {

            // The buffer may be shared with another image, in which case we do not destroy it now.
            bool channelBufferIsNotShared =
            ((_imp->channels[i].use_count() == 1) || (_imp->cacheEntry && _imp->channels[i].use_count() == 2));

            if (channelBufferIsNotShared) {
                toDeleteInDeleterThread.push_back(_imp->channels[i]);
            }
            assert(_imp->channels[i].use_count() > 1);
        }

    }

    if (!toDeleteInDeleterThread.empty()) {
        appPTR->deleteCacheEntriesInSeparateThread(toDeleteInDeleterThread);
    }

}

Image::InitStorageArgs::InitStorageArgs()
: bounds()
, perMipMapPixelRoD()
, storage(eStorageModeRAM)
, bitdepth(eImageBitDepthFloat)
, plane(ImagePlaneDesc::getRGBAComponents())
, cachePolicy(eCacheAccessModeNone)
, createTilesMapEvenIfNoCaching(false)
, bufferFormat(eImageBufferLayoutRGBAPackedFullRect)
, proxyScale(1.)
, mipMapLevel(0)
, isDraft(false)
, nodeTimeViewVariantHash(0)
, glContext()
, textureTarget(GL_TEXTURE_2D)
, externalBuffer()
, delayAllocation(false)
{

}


struct ActionStatusWrapper
{
    ActionRetCodeEnum stat;
    mutable QMutex statMutex;

    void updateStatus(ActionRetCodeEnum status)
    {
        QMutexLocker k(&statMutex);
        if (isFailureRetCode(stat)) {
            // If it's already failed, don't update
            return;
        }
        stat = status;
    }

    bool isFailed() const
    {
        QMutexLocker k(&statMutex);
        return isFailureRetCode(stat);
    }
};
typedef boost::shared_ptr<ActionStatusWrapper> StatPtr;


ActionRetCodeEnum
ImagePrivate::init(const Image::InitStorageArgs& args)
{

    // The bounds of the image must not be empty
    if (args.bounds.isNull()) {
        return eActionStatusFailed;
    }

    RenderScale proxyPlusMipMapScale = args.proxyScale;
    {
        double mipMapScale = Image::getScaleFromMipMapLevel(args.mipMapLevel);
        proxyPlusMipMapScale.x *= mipMapScale;
        proxyPlusMipMapScale.y *= mipMapScale;
    }

    originalBounds = args.bounds;
    cachePolicy = args.cachePolicy;
    if (args.storage != eStorageModeRAM) {
        // We can only cache stuff on disk.
        cachePolicy = eCacheAccessModeNone;
    }
    bufferFormat = args.bufferFormat;
    plane = args.plane;
    proxyScale = args.proxyScale;
    mipMapLevel = args.mipMapLevel;
    renderClone = args.renderClone;
    bitdepth = args.bitdepth;
    tilesAllocated = !args.delayAllocation;
    storage = args.storage;

    // OpenGL texture back-end only supports 32-bit float RGBA packed format.
    assert(args.storage != eStorageModeGLTex || (args.bufferFormat == eImageBufferLayoutRGBAPackedFullRect && args.bitdepth == eImageBitDepthFloat));
    if (args.storage == eStorageModeGLTex && (args.bufferFormat != eImageBufferLayoutRGBAPackedFullRect || args.bitdepth != eImageBitDepthFloat)) {
        return eActionStatusFailed;
    }


    // If allocating OpenGL textures, ensure the context is current
    boost::scoped_ptr<OSGLContextSaver> contextSaver;
    {
        OSGLContextAttacherPtr contextLocker;
        if (args.storage == eStorageModeGLTex) {
            contextSaver.reset(new OSGLContextSaver);
            contextLocker = OSGLContextAttacher::create(args.glContext);
            contextLocker->attach();
        }

        if (args.externalBuffer) {
            return initFromExternalBuffer(args);
        } else {
            return initAndFetchFromCache(args);
        }
    } // contextLocker
} // init


Image::CopyPixelsArgs::CopyPixelsArgs()
: roi()
, conversionChannel(0)
, alphaHandling(Image::eAlphaChannelHandlingFillFromChannel)
, monoConversion(Image::eMonoToPackedConversionCopyToChannelAndLeaveOthers)
, srcColorspace(eViewerColorSpaceLinear)
, dstColorspace(eViewerColorSpaceLinear)
, unPremultIfNeeded(false)
, forceCopyEvenIfBuffersHaveSameLayout(false)
{

}

#if 0
static bool isCopyPixelsNeeded(ImagePrivate* thisImage, ImagePrivate* otherImage)
{
    if ((thisImage->storage == eStorageModeGLTex || otherImage->storage == eStorageModeGLTex)) {
        return true;
    }

    if (thisImage->originalBounds != otherImage->originalBounds) {
        return true;
    }
    if (thisImage->bitdepth != otherImage->bitdepth) {
        return true;
    }
    if (thisImage->plane.getNumComponents() != otherImage->plane.getNumComponents()) {
        return true;
    }
    // Only support copying buffers with different layouts if they have 1 component only
    if (thisImage->bufferFormat != otherImage->bufferFormat && thisImage->plane.getNumComponents() != 1) {
        return true;
    }

    return false;
}
#endif

ActionRetCodeEnum
Image::copyPixels(const Image& other, const CopyPixelsArgs& args)
{

    // First intersect the RoI with the destination image. If it does not, do nothing.
    RectI roi;
    if (!_imp->originalBounds.intersect(args.roi, &roi)) {
        return eActionStatusOK;
    }

    // If the source image does not contain entirely the roi, we must clear to black
    // the roi in the destination image first
    if (!other._imp->originalBounds.contains(roi)) {
        ActionRetCodeEnum stat = fillZero(roi);
        if (isFailureRetCode(stat)) {
            return stat;
        }
    }

    // Intersect the roi with the source image bounds to figure out the remaining portion to copy
    if (!other._imp->originalBounds.intersect(roi, &roi)) {
        return eActionStatusOK;
    }

    assert(_imp->tilesAllocated && other._imp->tilesAllocated);

#ifdef DEBUG
    if (_imp->proxyScale.x != other._imp->proxyScale.x ||
        _imp->proxyScale.y != other._imp->proxyScale.y ||
        _imp->mipMapLevel != other._imp->mipMapLevel) {
        qDebug() << "Warning: attempt to call copyPixels on images with different scale";
    }
#endif

#if 0
    // Optimize: try to just copy the memory buffer pointers instead of copying the memory itself
    if (!args.forceCopyEvenIfBuffersHaveSameLayout && roi == _imp->originalBounds && roi == other._imp->originalBounds) {
        bool copyNeeded = isCopyPixelsNeeded(_imp.get(), other._imp.get());
        if (!copyNeeded) {
            for (std::size_t c = 0; c < 4; ++c) {
                if (_imp->channels[c]) {
                    assert(_imp->channels[c]->canSoftCopy(*other._imp->channels[c]));
                    _imp->channels[c]->softCopy(*other._imp->channels[c]);
                }
            }
            return eActionStatusOK;
        }

    }
#endif
    ImagePtr tmpImage;
    {
        ActionRetCodeEnum stat = ImagePrivate::checkIfCopyToTempImageIsNeeded(other, *this, roi, &tmpImage);
        if (isFailureRetCode(stat)) {
            return stat;
        }
    }

    const Image* fromImage = tmpImage? tmpImage.get() : &other;

    assert(_imp->originalBounds.contains(roi) && other._imp->originalBounds.contains(roi));

    // Ensure the channelIndex to copy from/to is valid according to the number of components of the image
    CopyPixelsArgs tmpArgs = args;
    tmpArgs.roi = roi;
    if (tmpArgs.alphaHandling == eAlphaChannelHandlingFillFromChannel) {
        if (tmpArgs.conversionChannel < 0 || tmpArgs.conversionChannel >= (int)fromImage->getComponentsCount()) {
            if (fromImage->getComponentsCount() == 4) {
                tmpArgs.conversionChannel = 3;
            } else {
                tmpArgs.conversionChannel = 0;
            }
        }
    }

    return ImagePrivate::copyPixelsInternal(fromImage->_imp.get(), _imp.get(), tmpArgs, _imp->renderClone.lock());

} // copyPixels

bool
Image::isBufferAllocated() const
{
    QMutexLocker k(&_imp->tilesAllocatedMutex);
    return _imp->tilesAllocated;
}

void
Image::ensureBuffersAllocated()
{
    QMutexLocker k(&_imp->tilesAllocatedMutex);
    if (_imp->tilesAllocated) {
        return;
    }

    for (int i = 0; i < 4; ++i) {
        if (!_imp->channels[i]) {
            continue;
        }
        if (_imp->channels[i]->isAllocated()) {
            // The buffer is already allocated
            continue;
        }
        if (_imp->channels[i]->hasAllocateMemoryArgs()) {
            // Allocate the buffer
            _imp->channels[i]->allocateMemoryFromSetArgs();
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
    return _imp->channels[0]->getStorageMode();
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
    return _imp->plane.getNumComponents();
}

const ImagePlaneDesc&
Image::getLayer() const
{
    return _imp->plane;
}


ImageBitDepthEnum
Image::getBitDepth() const
{
    return _imp->bitdepth;
}

GLImageStoragePtr
Image::getGLImageStorage() const
{
    return toGLImageStorage(_imp->channels[0]);
}

void
Image::getCPUData(CPUData* data) const
{
    ImagePrivate* imp = _imp.get();
    ImagePrivate::getCPUDataInternal(imp->originalBounds, imp->plane.getNumComponents(), imp->channels, imp->bitdepth, imp->bufferFormat, data);
}



CacheAccessModeEnum
Image::getCachePolicy() const
{
    return _imp->cachePolicy;
}

ImageCacheEntryPtr
Image::getCacheEntry() const
{
    return _imp->cacheEntry;
}

class FillProcessor : public ImageMultiThreadProcessorBase
{
    void* _ptrs[4];
    RectI _bounds;
    ImageBitDepthEnum _bitDepth;
    int _nComps;
    RGBAColourF _color;

public:

    FillProcessor(const EffectInstancePtr& renderClone)
    : ImageMultiThreadProcessorBase(renderClone)
    {

    }

    virtual ~FillProcessor()
    {
    }

    void setValues(void* ptrs[4], const RectI& bounds, ImageBitDepthEnum bitDepth, int nComps, const RGBAColourF& color)
    {
        memcpy(_ptrs, ptrs, sizeof(void*) * 4);
        _bounds = bounds;
        _bitDepth = bitDepth;
        _nComps = nComps;
        _color = color;
    }

private:

    virtual ActionRetCodeEnum multiThreadProcessImages(const RectI& renderWindow) OVERRIDE FINAL
    {
        return ImagePrivate::fillCPU(_ptrs, _color.r, _color.g, _color.b, _color.a, _nComps, _bitDepth, _bounds, renderWindow, _effect);
    }
};

ActionRetCodeEnum
Image::fill(const RectI & roi,
            float r,
            float g,
            float b,
            float a)
{

    if (getStorageMode() == eStorageModeGLTex) {
        GLImageStoragePtr glEntry = toGLImageStorage(_imp->channels[0]);
        _imp->fillGL(roi, r, g, b, a, glEntry->getTexture(), glEntry->getOpenGLContext());
        return eActionStatusOK;
    }



    Image::CPUData data;
    getCPUData(&data);
    RectI clippedRoi;
    roi.intersect(data.bounds, &clippedRoi);

    RGBAColourF color = {r, g, b, a};

    FillProcessor processor(_imp->renderClone.lock());
    processor.setValues(data.ptrs, data.bounds, data.bitDepth, data.nComps, color);
    processor.setRenderWindow(clippedRoi);
    return processor.process();



} // fill

ActionRetCodeEnum
Image::fillZero(const RectI& roi)
{
    return fill(roi, 0., 0., 0., 0.);
}

ActionRetCodeEnum
Image::fillBoundsZero()
{
    return fillZero(getBounds());
}

void
Image::updateRenderCloneAndImage(const EffectInstancePtr& newRenderClone)
{
    _imp->renderClone = newRenderClone;
    if (_imp->cacheEntry) {
        _imp->cacheEntry->updateRenderCloneAndImage(shared_from_this(), newRenderClone);
    }
}

ActionRetCodeEnum
Image::ensureBounds(const RectI& roi,
                    unsigned int mipmapLevel,
                    const std::vector<RectI>& perMipMapLevelRoDPixel,
                    const EffectInstancePtr& newRenderClone)
{
    if (_imp->originalBounds.contains(roi)) {
        return eActionStatusOK;
    }

    ensureBuffersAllocated();

    RectI oldBounds = _imp->originalBounds;
    RectI newBounds = oldBounds;
    newBounds.merge(roi);

    ImagePtr tmpImage;
    {
        Image::InitStorageArgs initArgs;
        initArgs.bounds = newBounds;
        initArgs.plane = getLayer();
        initArgs.bitdepth = getBitDepth();
        initArgs.bufferFormat = getBufferFormat();
        initArgs.storage = getStorageMode();
        initArgs.mipMapLevel = getMipMapLevel();
        initArgs.renderClone = newRenderClone;
        assert(mipmapLevel == initArgs.mipMapLevel);
        initArgs.proxyScale = getProxyScale();
        initArgs.renderClone = _imp->renderClone.lock();
        GLImageStoragePtr isGlEntry = getGLImageStorage();
        if (isGlEntry) {
            initArgs.textureTarget = isGlEntry->getGLTextureTarget();
            initArgs.glContext = isGlEntry->getOpenGLContext();
        }
        tmpImage = Image::create(initArgs);
        if (!tmpImage) {
            return eActionStatusFailed;
        }
/*
        float fillValue = isGlEntry ? 0 : std::numeric_limits<float>::quiet_NaN();
        ActionRetCodeEnum stat = tmpImage->fill(initArgs.bounds, fillValue, fillValue, fillValue, fillValue);
*/
        ActionRetCodeEnum stat = tmpImage->fillBoundsZero();
        if (isFailureRetCode(stat)) {
            return stat;
        }

    }
    Image::CopyPixelsArgs cpyArgs;
    cpyArgs.roi = oldBounds;
    ActionRetCodeEnum stat = tmpImage->copyPixels(*this, cpyArgs);
    if (isFailureRetCode(stat)) {
        return stat;
    }

    // Swap images so that this image becomes the resized one, but keep the internal cache entry object
    ImageCacheEntryPtr internalCacheEntry = _imp->cacheEntry;
    CacheAccessModeEnum cachePolicy = getCachePolicy();
    _imp.swap(tmpImage->_imp);
    _imp->cachePolicy = cachePolicy;
    _imp->_publicInterface = this;
    if (internalCacheEntry) {
        assert(perMipMapLevelRoDPixel.size() >= _imp->mipMapLevel + 1);
        _imp->cacheEntry = internalCacheEntry;
        _imp->cacheEntry->ensureRoI(roi, _imp->channels, perMipMapLevelRoDPixel);
        _imp->cacheEntry->updateRenderCloneAndImage(shared_from_this(), newRenderClone);
    }
    return eActionStatusOK;

} // ensureBounds

ActionRetCodeEnum
Image::fillOutSideWithBlack(const RectI& roi)
{
    RectI clipped;
    _imp->originalBounds.intersect(roi, &clipped);

    RectI fourRects[4];
    ImageTilesState::getABCDRectangles(clipped, _imp->originalBounds, fourRects[0], fourRects[1], fourRects[2], fourRects[3]);
    for (int i = 0; i < 4; ++i) {
        if (fourRects[i].isNull()) {
            continue;
        }
        ActionRetCodeEnum stat = fillZero(fourRects[i]);
        if (isFailureRetCode(stat)) {
            return stat;
        }
    }
    return eActionStatusOK;
}

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

    // The roi must be contained in the bounds of the image
    assert(_imp->originalBounds.contains(roi));
    if (!_imp->originalBounds.contains(roi)) {
        return ImagePtr();
    }

    RectI dstRoI  = roi.downscalePowerOfTwoSmallestEnclosing(downscaleLevels);
    ImagePtr mipmapImage;

    RectI previousLevelRoI = roi;
    ImageConstPtr previousLevelImage = shared_from_this();

    // Build all the mipmap levels until we reach the one we are interested in
    for (unsigned int i = 0; i < downscaleLevels; ++i) {

        // Halve the smallest enclosing po2 rect as we need to render a minimum of the renderWindow
        RectI halvedRoI = previousLevelRoI.downscalePowerOfTwoSmallestEnclosing(1);

        // Allocate an image with half the size of the source image
        {
            InitStorageArgs args;
            args.bounds = halvedRoI;
            args.renderClone = _imp->renderClone.lock();
            args.plane = previousLevelImage->_imp->plane;
            args.bitdepth = previousLevelImage->getBitDepth();
            args.proxyScale = previousLevelImage->getProxyScale();
            args.mipMapLevel = previousLevelImage->getMipMapLevel() + 1;
            mipmapImage = Image::create(args);
            if (!mipmapImage) {
                return mipmapImage;
            }
        }

        Image::CPUData srcTileData;
        previousLevelImage->getCPUData(&srcTileData);

        Image::CPUData dstTileData;
        mipmapImage->getCPUData(&dstTileData);

        ActionRetCodeEnum stat = ImagePrivate::halveImage((const void**)srcTileData.ptrs, srcTileData.nComps, srcTileData.bitDepth, srcTileData.bounds, dstTileData.ptrs, dstTileData.bounds, _imp->renderClone.lock());
        if (isFailureRetCode(stat)) {
            return ImagePtr();
        }

        // Switch for next pass
        previousLevelRoI = halvedRoI;
        previousLevelImage = mipmapImage;
    } // for all downscale levels
    return mipmapImage;

} // downscaleMipMap


class CheckNaNsProcessor : public ImageMultiThreadProcessorBase
{

    Image::CPUData _data;
    mutable QMutex _foundNaNMutex;
    bool _foundNan;

public:

    CheckNaNsProcessor(const EffectInstancePtr& renderClone)
    : ImageMultiThreadProcessorBase(renderClone)
    , _data()
    , _foundNan(false)
    {

    }

    virtual ~CheckNaNsProcessor()
    {
    }

    void setValues(const Image::CPUData& data)
    {
        _data = data;
    }

    bool hasNaN() const
    {
        QMutexLocker k(&_foundNaNMutex);
        return _foundNan;
    }

private:

    virtual ActionRetCodeEnum multiThreadProcessImages(const RectI& renderWindow) OVERRIDE FINAL
    {
        bool foundNaN = false;
        ActionRetCodeEnum stat = ImagePrivate::checkForNaNs(_data.ptrs, _data.nComps, _data.bitDepth, _data.bounds, renderWindow, _effect, &foundNaN);
        if (foundNaN) {
            QMutexLocker k(&_foundNaNMutex);
            _foundNan = true;
        }
        return stat;
    }
};

ActionRetCodeEnum
Image::checkForNaNs(const RectI& roi, bool* foundNan)
{
    if (getBitDepth() != eImageBitDepthFloat) {
        return eActionStatusFailed;
    }
    if (getStorageMode() == eStorageModeGLTex) {
        return eActionStatusFailed;
    }

    Image::CPUData data;
    getCPUData(&data);

    RectI clippedRoi;
    roi.intersect(data.bounds, &clippedRoi);

    CheckNaNsProcessor processor(_imp->renderClone.lock());
    processor.setValues(data);
    processor.setRenderWindow(clippedRoi);
    ActionRetCodeEnum stat = processor.process();
    *foundNan = processor.hasNaN();
    return stat;

} // checkForNaNs


class MaskMixProcessor : public ImageMultiThreadProcessorBase
{
    Image::CPUData _srcTileData, _maskTileData, _dstTileData;
    double _mix;
    bool _maskInvert;
public:

    MaskMixProcessor(const EffectInstancePtr& renderClone)
    : ImageMultiThreadProcessorBase(renderClone)
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

    void setValues(const Image::CPUData& srcTileData,
                   const Image::CPUData& maskTileData,
                   const Image::CPUData& dstTileData,
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

    virtual ActionRetCodeEnum multiThreadProcessImages(const RectI& renderWindow) OVERRIDE FINAL
    {
        return ImagePrivate::applyMaskMixCPU((const void**)_srcTileData.ptrs,
                                             _srcTileData.bounds,
                                             _srcTileData.nComps,
                                             (const void**)_maskTileData.ptrs,
                                             _maskTileData.bounds, _dstTileData.ptrs,
                                             _dstTileData.bitDepth,
                                             _dstTileData.nComps,
                                             _mix,
                                             _maskInvert,
                                             _dstTileData.bounds,
                                             renderWindow,
                                             _effect);
    }
};

ActionRetCodeEnum
Image::applyMaskMix(const RectI& roi,
                    const ImagePtr& maskImg,
                    const ImagePtr& originalImg,
                    bool masked,
                    bool maskInvert,
                    float mix)
{
    // !masked && mix == 1: nothing to do
    if ( !masked && (mix == 1) ) {
        return eActionStatusOK;
    }

    // Mask must be alpha
    assert( !masked || !maskImg || maskImg->getLayer().getNumComponents() == 1 );

    if (getStorageMode() == eStorageModeGLTex) {

        GLImageStoragePtr originalImageTexture, maskTexture, dstTexture;
        if (originalImg) {
            assert(originalImg->getStorageMode() == eStorageModeGLTex);
            originalImageTexture = toGLImageStorage(originalImg->_imp->channels[0]);
        }
        if (maskImg && masked) {
            assert(maskImg->getStorageMode() == eStorageModeGLTex);
            maskTexture = toGLImageStorage(maskImg->_imp->channels[0]);
        }
        dstTexture = toGLImageStorage(_imp->channels[0]);
        ImagePrivate::applyMaskMixGL(originalImageTexture, maskTexture, dstTexture, mix, maskInvert, roi);
        return eActionStatusOK;
    }

    // This function only works if original image and mask image have the same bitdepth as output
    assert(!originalImg || (originalImg->getBitDepth() == getBitDepth()));
    assert(!maskImg || (maskImg->getBitDepth() == getBitDepth()));

    Image::CPUData srcImgData, maskImgData;
    if (originalImg) {
        originalImg->getCPUData(&srcImgData);
    }

    if (maskImg) {
        maskImg->getCPUData(&maskImgData);
        assert(maskImgData.nComps == 1);
    }

    Image::CPUData dstImgData;
    getCPUData(&dstImgData);

    RectI tileRoI;
    roi.intersect(dstImgData.bounds, &tileRoI);

    MaskMixProcessor processor(_imp->renderClone.lock());
    processor.setValues(srcImgData, maskImgData, dstImgData, mix, maskInvert);
    processor.setRenderWindow(tileRoI);
    return processor.process();

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
    Image::CPUData _srcImgData, _dstImgData;
    std::bitset<4> _processChannels;
public:

    CopyUnProcessedProcessor(const EffectInstancePtr& renderClone)
    : ImageMultiThreadProcessorBase(renderClone)
    {

    }

    virtual ~CopyUnProcessedProcessor()
    {
    }

    void setValues(const Image::CPUData& srcImgData,
                   const Image::CPUData& dstImgData,
                   std::bitset<4> processChannels)
    {
        _srcImgData = srcImgData;
        _dstImgData = dstImgData;
        _processChannels = processChannels;
    }

private:

    virtual ActionRetCodeEnum multiThreadProcessImages(const RectI& renderWindow) OVERRIDE FINAL
    {
        return ImagePrivate::copyUnprocessedChannelsCPU((const void**)_srcImgData.ptrs, _srcImgData.bounds, _srcImgData.nComps, (void**)_dstImgData.ptrs, _dstImgData.bitDepth, _dstImgData.nComps, _dstImgData.bounds, _processChannels, renderWindow, _effect);
    }
};


ActionRetCodeEnum
Image::copyUnProcessedChannels(const RectI& roi,
                               const std::bitset<4> processChannels,
                               const ImagePtr& originalImg)
{

    if (!canCallCopyUnProcessedChannels(processChannels)) {
        return eActionStatusOK;
    }

    if (getStorageMode() == eStorageModeGLTex) {

        GLImageStoragePtr originalImageTexture, dstTexture;
        if (originalImg) {
            assert(originalImg->getStorageMode() == eStorageModeGLTex);
            originalImageTexture = toGLImageStorage(originalImg->_imp->channels[0]);
        }

        dstTexture = toGLImageStorage(_imp->channels[0]);

        RectI realRoi;
        roi.intersect(dstTexture->getBounds(), &realRoi);
        ImagePrivate::copyUnprocessedChannelsGL(originalImageTexture, dstTexture, processChannels, realRoi);
        return eActionStatusOK;
    }

    // This function only works if original  image has the same bitdepth as output
    assert(!originalImg || (originalImg->getBitDepth() == getBitDepth()));

    Image::CPUData srcImgData;
    if (originalImg) {
        originalImg->getCPUData(&srcImgData);
    }


    Image::CPUData dstImgData;
    getCPUData(&dstImgData);

    RectI tileRoI;
    roi.intersect(dstImgData.bounds, &tileRoI);

    CopyUnProcessedProcessor processor(_imp->renderClone.lock());
    processor.setValues(srcImgData, dstImgData, processChannels);
    processor.setRenderWindow(tileRoI);
    return processor.process();

} // copyUnProcessedChannels

class ApplyPixelShaderProcessor : public ImageMultiThreadProcessorBase
{
    Image::CPUData  _dstImgData;
    const void* _customData;
    void(*_func)();
public:

    ApplyPixelShaderProcessor(const EffectInstancePtr& renderClone)
    : ImageMultiThreadProcessorBase(renderClone)
    {

    }

    virtual ~ApplyPixelShaderProcessor()
    {
    }

    void setValues(const Image::CPUData& dstImgData, void(*func)(), const void* customData)
    {
        _func = func;
        _dstImgData = dstImgData;
        _customData = customData;
    }

private:

    virtual ActionRetCodeEnum multiThreadProcessImages(const RectI& renderWindow) OVERRIDE FINAL
    {
        return ImagePrivate::applyCPUPixelShader(renderWindow, _customData, _effect, _dstImgData, _func);
    }
};


ActionRetCodeEnum
ImagePrivate::applyCPUPixelShaderForDepth(const RectI& roi,
                           const void* customData,
                           void(*func)())
{
    if (storage != eStorageModeRAM) {
        return eActionStatusFailed;
    }
    Image::CPUData dstImgData;
    _publicInterface->getCPUData(&dstImgData);

    RectI tileRoI;
    roi.intersect(dstImgData.bounds, &tileRoI);

    ApplyPixelShaderProcessor processor(renderClone.lock());
    processor.setValues(dstImgData, func, customData);
    processor.setRenderWindow(tileRoI);
    return processor.process();
}

ActionRetCodeEnum
Image::applyCPUPixelShader_Float(const RectI& roi, const void* customData, ImageCPUPixelShaderFloat func)
{
    if (getBitDepth() != eImageBitDepthFloat) {
        return eActionStatusFailed;
    }
    return _imp->applyCPUPixelShaderForDepth(roi, customData, (void(*)())func);
}

ActionRetCodeEnum
Image::applyCPUPixelShader_Short(const RectI& roi, const void* customData, ImageCPUPixelShaderShort func)
{
    if (getBitDepth() != eImageBitDepthShort) {
        return eActionStatusFailed;
    }
    return _imp->applyCPUPixelShaderForDepth(roi, customData, (void(*)())func);
}

ActionRetCodeEnum
Image::applyCPUPixelShader_Byte(const RectI& roi, const void* customData, ImageCPUPixelShaderByte func)
{
    if (getBitDepth() != eImageBitDepthByte) {
        return eActionStatusFailed;
    }
    return _imp->applyCPUPixelShaderForDepth(roi, customData, (void(*)())func);
}


NATRON_NAMESPACE_EXIT
