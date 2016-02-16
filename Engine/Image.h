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

#include <list>
#include <map>
#include <algorithm> // min, max
#include <bitset>

#include "Global/GlobalDefines.h"

CLANG_DIAG_OFF(deprecated)
#include <QtCore/QHash>
CLANG_DIAG_ON(deprecated)
#include <QtCore/QReadWriteLock>

#include "Engine/ImageKey.h"
#include "Engine/ImageComponents.h"
#include "Engine/ImageParams.h"
#include "Engine/CacheEntry.h"
#include "Engine/OutputSchedulerThread.h"
#include "Engine/RectD.h"
#include "Engine/ViewIdx.h"
#include "Engine/EngineFwd.h"


NATRON_NAMESPACE_ENTER;


class GenericAccess
{
public:

    GenericAccess() {}

    virtual ~GenericAccess() {

    }
};

class Bitmap
{
public:
    Bitmap(const RectI & bounds)
    : _bounds(bounds)
    , _map( bounds.area() )
    , _dirtyZone()
    , _dirtyZoneSet(false)
    {
        //Do not assert !rod.isNull() : An empty image can be created for entries that correspond to
        // "identities" images (i.e: images that are just a link to another image). See EffectInstance :
        // "!!!Note that if isIdentity is true it will allocate an empty image object with 0 bytes of data."
        //assert(!rod.isNull());
        std::fill(_map.begin(), _map.end(), 0);
    }

    Bitmap()
    : _bounds()
    , _map()
    , _dirtyZone()
    , _dirtyZoneSet(false)
    {
    }

    void initialize(const RectI & bounds)
    {
        _bounds = bounds;
        _map.resize( _bounds.area() );

        std::fill(_map.begin(), _map.end(), 0);
    }

    ~Bitmap()
    {
    }


    void setTo1()
    {
        std::fill(_map.begin(),_map.end(),1);
    }

    const RectI & getBounds() const
    {
        return _bounds;
    }

#if NATRON_ENABLE_TRIMAP
    void minimalNonMarkedRects_trimap(const RectI & roi,std::list<RectI>& ret,bool* isBeingRenderedElsewhere) const;
    RectI minimalNonMarkedBbox_trimap(const RectI & roi,bool* isBeingRenderedElsewhere) const;
#endif

    void minimalNonMarkedRects(const RectI & roi,std::list<RectI>& ret) const;
    RectI minimalNonMarkedBbox(const RectI & roi) const;


    ///Fill with 1 the roi
    void markForRendered(const RectI & roi);

#if NATRON_ENABLE_TRIMAP
    ///Fill with 2 the roi
    void markForRendering(const RectI & roi);
#endif

    void clear(const RectI& roi);

    void swap(Bitmap& other);

    const char* getBitmap() const
    {
        return &_map.front();
    }

    char* getBitmap()
    {
        return &_map.front();
    }

    const char* getBitmapAt(int x,int y) const;
    char* getBitmapAt(int x,int y);

    void copyRowPortion(int x1,int x2,int y,const Bitmap& other);

    void copyBitmapPortion(const RectI& roi, const Bitmap& other);

    void setDirtyZone(const RectI& zone) {
        _dirtyZone = zone;
        _dirtyZoneSet = true;
    }

private:
    RectI _bounds;
    std::vector<char> _map;

    /**
     * This represents the zone that has potentially something to render. In minimalNonMarkedRects
     * we intersect the region of interest with the dirty zone. This is useful to optimize the bitmap checking
     * when we are sure multiple threads are not using the image and we have a very small RoI to render.
     * For now it's only used for the rotopaint while painting.
     **/
    RectI _dirtyZone;
    bool _dirtyZoneSet;
};

class Image
: public CacheEntryHelper<unsigned char,ImageKey,ImageParams> , public BufferableObject
{
public:

    Image(const ImageKey & key,
          const boost::shared_ptr<ImageParams> &  params,
          const CacheAPI* cache,
          StorageModeEnum storage,
          const std::string & path);



    /*This constructor can be used to allocate a local Image. The deallocation should
     then be handled by the user. Note that no view number is passed in parameter
     as it is not needed.*/
    Image(const ImageComponents& components,
          const RectD & regionOfDefinition,    //!< rod in canonical coordinates
          const RectI & bounds,    //!< bounds in pixel coordinates
          unsigned int mipMapLevel,
          double par,
          ImageBitDepthEnum bitdepth,
          bool useBitmap = false);

    //Same as above but parameters are in the ImageParams object
    Image(const ImageKey & key,
          const boost::shared_ptr<ImageParams>& params);


    virtual ~Image();

    bool usesBitMap() const { return _useBitmap; }

    virtual void onMemoryAllocated(bool diskRestoration) OVERRIDE FINAL;

    static ImageKey makeKey(const CacheEntryHolder* holder,
                            U64 nodeHashKey,
                            bool frameVaryingOrAnimated,
                            double time,
                            ViewIdx view,
                            bool draftMode,
                            bool fullScaleWithDownscaleInputs);
    static boost::shared_ptr<ImageParams> makeParams(int cost,
                                                     const RectD & rod,    // the image rod in canonical coordinates
                                                     const double par,
                                                     unsigned int mipMapLevel,
                                                     bool isRoDProjectFormat,
                                                     const ImageComponents& components,
                                                     ImageBitDepthEnum bitdepth);

    static boost::shared_ptr<ImageParams> makeParams(int cost,
                                                     const RectD & rod,    // the image rod in canonical coordinates
                                                     const RectI& bounds,
                                                     const double par,
                                                     unsigned int mipMapLevel,
                                                     bool isRoDProjectFormat,
                                                     const ImageComponents& components,
                                                     ImageBitDepthEnum bitdepth);

    // boost::shared_ptr<ImageParams> getParams() const WARN_UNUSED_RETURN;

    /**
     * @brief Resizes this image so it contains newBounds, copying all the content of the current bounds of the image into
     * a new buffer. This is not thread-safe and should be called only while under an ImageLocker
     **/
    bool ensureBounds(const RectI& newBounds, bool fillWithBlackAndTransparent = false, bool setBitmapTo1 = false);

    /**
     * @brief Same as ensureBounds() except that if a resize is needed, it will do the resize in the output image instead to avoid taking the
     * write lock from this image.
     **/
    bool copyAndResizeIfNeeded(const RectI& newBounds, bool fillWithBlackAndTransparent, bool setBitmapTo1, boost::shared_ptr<Image>* output);

private:

    static void resizeInternal(const Image* srcImg,
                               const RectI& srcBounds,
                               const RectI& merge,
                               bool fillWithBlackAndTransparent,
                               bool setBitmapTo1,
                               bool createInCache,
                               boost::shared_ptr<Image>* outputImage);


public:

    /**
     * @brief Returns the region of definition of the image in canonical coordinates. It doesn't have any
     * scale applied to it. In order to return the true pixel data window you must call getBounds()
     * WARNING: this is NOT the same definition as in OpenFX, where the Image RoD is always in pixels.
     **/
    const RectD & getRoD() const
    {
        return _rod;
    };

    /**
     * @brief Do not use this. This is used only to circumvent a situation where 2 images of the same hash could have a different RoD
     * to prevent an assert from triggering.
     **/
    void setRoD(const RectD& rod);

    /**
     * @brief Returns the bounds where data is in the image.
     * This is equivalent to calling getRoD().mipMapLevel(getMipMapLevel());
     * but slightly faster since it is stored as a member of the image.
     **/
    RectI getBounds() const
    {
        QReadLocker k(&_entryLock);
        return _bounds;
    };
    virtual size_t size() const OVERRIDE FINAL
    {
        std::size_t dt = dataSize();

        bool got = _entryLock.tryLockForRead();
        dt += _bitmap.getBounds().area();
        if (got) {
            _entryLock.unlock();
        }
        return dt;
    }


    ///Overriden from BufferableObject
    virtual std::size_t sizeInRAM() const OVERRIDE FINAL
    {
        return size();
    }

    unsigned int getMipMapLevel() const
    {
        return this->_params->getMipMapLevel();
    }

    double getScale() const
    {
        return getScaleFromMipMapLevel(getMipMapLevel());
    }

    unsigned int getComponentsCount() const;

    const ImageComponents& getComponents() const
    {
        return this->_params->getComponents();
    }

    void setBitmapDirtyZone(const RectI& zone);

    /**
     * @brief This function returns true if the components 'from' have enough components to
     * convert to the 'to' components.
     * e.g: RGBA to RGB would return true , the opposite would return false.
     **/
    static bool hasEnoughDataToConvert(ImageComponentsEnum from, ImageComponentsEnum to);
    static std::string getFormatString(const ImageComponents& comps, ImageBitDepthEnum depth);
    static std::string getDepthString(ImageBitDepthEnum depth);
    static bool isBitDepthConversionLossy(ImageBitDepthEnum from, ImageBitDepthEnum to);
    ImageBitDepthEnum getBitDepth() const
    {
        return this->_bitDepth;
    }

    double getPixelAspectRatio() const;


    /**
     * @brief Same as getElementsCount(getComponents()) * getBounds().width()
     **/
    unsigned int getRowElements() const;



    /**
     * @brief Lock the image for reading, while this object is living, the image buffer can't be written to.
     * You must ensure that the image will live as long as this object lives otherwise the pointer will be invalidated.
     * You may no longer use the pointer returned by pixelAt once this object dies.
     **/
    class ReadAccess : public GenericAccess
    {
        const Image* img;
    public:

        ReadAccess(const Image* img)
        : GenericAccess()
        , img(img)
        {
            if (img) {
                img->lockForRead();
            }
        }

        ReadAccess(const ReadAccess& other)
        : GenericAccess()
        , img(other.img)
        {
            //This is a recursive lock so it doesn't matter if we take it twice
            if (img) {
                img->lockForRead();
            }
        }

        virtual ~ReadAccess()
        {
            if (img) {
                img->unlock();
            }
        }

        /**
         * @brief Access pixels. The pointer must be cast to the appropriate type afterwards.
         **/
        const unsigned char* pixelAt(int x,int y) const
        {
            assert(img);
            return img->pixelAt(x, y);
        }

        const char* bitmapAt(int x, int y) const
        {
            assert(img);
            return img->getBitmapAt(x, y);
        }
    };

    /**
     * @brief Lock the image for writing, while this object is living, the image buffer can't be read.
     * You must ensure that the image will live as long as this object lives otherwise the pointer will be invalidated.
     * You may no longer use the pointer returned by pixelAt once this object dies.
     **/
    class WriteAccess : public GenericAccess
    {
        Image* img;
    public:

        WriteAccess(Image* img)
        : GenericAccess()
        , img(img)
        {
            img->lockForWrite();
        }

        WriteAccess(const WriteAccess& other)
        : GenericAccess()
        , img(other.img)
        {
            //This is a recursive lock so it doesn't matter if we take it twice
            img->lockForWrite();
        }

        virtual ~WriteAccess()
        {
            img->unlock();
        }

        /**
         * @brief Access pixels. The pointer must be cast to the appropriate type afterwards.
         **/
        unsigned char* pixelAt(int x,int y)
        {
            return img->pixelAt(x, y);
        }

        char* bitmapAt(int x, int y) const
        {
            assert(img);
            return img->getBitmapAt(x, y);
        }
    };

    ReadAccess getReadRights() const
    {
        return ReadAccess(this);
    }

    WriteAccess getWriteRights()
    {
        return WriteAccess(this);
    }

private:

    friend class ReadAccess;
    friend class WriteAccess;

    /**
     * These are private accessors to the buffer. They may only exclusively called while under the lock
     * of an image.
     **/

    const char* getBitmapAt(int x,
                            int y) const
    {
        return this->_bitmap.getBitmapAt(x,y);
    }

    char* getBitmapAt(int x,
                      int y)
    {
        return this->_bitmap.getBitmapAt(x,y);
    }

    /**
     * @brief Access pixels. The pointer must be cast to the appropriate type afterwards.
     **/
    unsigned char* pixelAt(int x,int y);
    const unsigned char* pixelAt(int x,int y) const;

    /**
     * @brief Locks the image for read/write access.
     * There can be a deadlock situation in the following situation:
     * The lock for read is taken when a plugin attempts to
     * fetch an image from a source clip. But if the plug-in
     * fetches twice the very same image (likely if this is a tracker on the last frame for example) then it will deadlock
     * if a clip is not asking for the exact same region because it is likely there is something left to render.
     *
     * We detect such calls and ensure that there are no deadlock.
     *
     *
     * NB: We cannot also rely on a mutex based implementation since it can lead to a deadlock in the following situation:
     * Thread A requests image at time T and locks it
     * Thread B requests image at time T+1 and locks it
     * Thread A requests image at time T+1 and hangs
     * Thread B requests image at time T and hangs
     **/
    void lockForRead() const
    {
        _entryLock.lockForRead();
    }
    void lockForWrite() const
    {
        _entryLock.lockForWrite();
    }

    void unlock() const
    {
        _entryLock.unlock();
    }


    template <typename SRCPIX,typename DSTPIX,int srcMaxValue,int dstMaxValue>
    static void
    convertToFormatInternal_sameComps(const RectI & renderWindow,
                                      const Image & srcImg,
                                      Image & dstImg,
                                      ViewerColorSpaceEnum srcColorSpace,
                                      ViewerColorSpaceEnum dstColorSpace,
                                      bool copyBitmap);

    template <typename SRCPIX,typename DSTPIX,int srcMaxValue,int dstMaxValue,int srcNComps,int dstNComps>
    static void
    convertToFormatInternal(const RectI & renderWindow,
                            const Image & srcImg,
                            Image & dstImg,
                            ViewerColorSpaceEnum srcColorSpace,
                            ViewerColorSpaceEnum dstColorSpace,
                            int channelForAlpha,
                            bool useAlpha0,
                            bool copyBitmap,
                            bool requiresUnpremult);



    template <typename SRCPIX,typename DSTPIX,int srcMaxValue,int dstMaxValue,int srcNComps,int dstNComps,
    bool requiresUnpremult>
    static void convertToFormatInternalForUnpremult(const RectI & renderWindow,
                                                    const Image & srcImg,
                                                    Image & dstImg,
                                                    ViewerColorSpaceEnum srcColorSpace,
                                                    ViewerColorSpaceEnum dstColorSpace,
                                                    bool useAlpha0,
                                                    bool copyBitmap,
                                                    int channelForAlpha);


    template <typename SRCPIX,typename DSTPIX,int srcMaxValue,int dstMaxValue,int srcNComps,int dstNComps,
    bool requiresUnpremult, bool useColorspaces>
    static void convertToFormatInternalForColorSpace(const RectI & renderWindow,
                                                     const Image & srcImg,
                                                     Image & dstImg,
                                                     bool copyBitmap,
                                                     bool useAlpha0,
                                                     ViewerColorSpaceEnum srcColorSpace,
                                                     ViewerColorSpaceEnum dstColorSpace,
                                                     int channelForAlpha);




    template <typename SRCPIX,typename DSTPIX,int srcMaxValue,int dstMaxValue>
    static void
    convertToFormatInternalForDepth(const RectI & renderWindow,
                                    const Image & srcImg,
                                    Image & dstImg,
                                    ViewerColorSpaceEnum srcColorSpace,
                                    ViewerColorSpaceEnum dstColorSpace,
                                    int channelForAlpha,
                                    bool useAlpha0,
                                    bool copyBitmap,
                                    bool requiresUnpremult);
public:


    /**
     * @brief Returns a list of portions of image that are not yet rendered within the
     * region of interest given. This internally uses the bitmap to know what portion
     * are already rendered in the image. It aims to return the minimal
     * area to render. Since this problem is quite hard to solve,the different portions
     * of image returned may contain already rendered pixels.
     *
     * Note that if the RoI is larger than the bounds of the image, the out of bounds portions
     * will be added to the resulting list of rectangles.
     **/
#if NATRON_ENABLE_TRIMAP
    void getRestToRender_trimap(const RectI & regionOfInterest,
                                std::list<RectI>& ret,
                                bool* isBeingRenderedElsewhere) const
    {
        if (!_useBitmap) {
            return;
        }
        QReadLocker locker(&_entryLock);
        _bitmap.minimalNonMarkedRects_trimap(regionOfInterest, ret, isBeingRenderedElsewhere);
    }
#endif
    void getRestToRender(const RectI & regionOfInterest,std::list<RectI>& ret) const
    {
        if (!_useBitmap) {
            return ;
        }
        QReadLocker locker(&_entryLock);
        _bitmap.minimalNonMarkedRects(regionOfInterest,ret);
    }

#if NATRON_ENABLE_TRIMAP
    RectI getMinimalRect_trimap(const RectI & regionOfInterest,bool* isBeingRenderedElsewhere) const
    {
        if (!_useBitmap) {
            return regionOfInterest;
        }
        QReadLocker locker(&_entryLock);
        return _bitmap.minimalNonMarkedBbox_trimap(regionOfInterest,isBeingRenderedElsewhere);
    }
#endif
    RectI getMinimalRect(const RectI & regionOfInterest) const
    {
        if (!_useBitmap) {
            return regionOfInterest;
        }
        QReadLocker locker(&_entryLock);
        return _bitmap.minimalNonMarkedBbox(regionOfInterest);
    }

#if NATRON_ENABLE_TRIMAP
    RectI getMinimalRectAndMarkForRendering_trimap(const RectI & regionOfInterest,bool* isBeingRenderedElsewhere)
    {
        if (!_useBitmap) {
            return regionOfInterest;
        }
        RectI ret;
        {
            QReadLocker locker(&_entryLock);
            ret = _bitmap.minimalNonMarkedBbox_trimap(regionOfInterest,isBeingRenderedElsewhere);
        }
        markForRendering(ret);
        return ret;
    }
#endif

    void markForRendered(const RectI & roi)
    {
        if (!_useBitmap) {
            return;
        }
        QWriteLocker locker(&_entryLock);
        RectI intersection;
        _bounds.intersect(roi, &intersection);
        _bitmap.markForRendered(intersection);
    }

#if NATRON_ENABLE_TRIMAP
    ///Fill with 2 the roi
    void markForRendering(const RectI & roi)
    {
        if (!_useBitmap) {
            return;
        }
        QWriteLocker locker(&_entryLock);
        RectI intersection;
        _bounds.intersect(roi, &intersection);
        _bitmap.markForRendering(intersection);
    }
#endif

    void clearBitmap(const RectI& roi)
    {
        if (!_useBitmap) {
            return;
        }
        QWriteLocker locker(&_entryLock);
        RectI intersection;
        _bounds.intersect(roi, &intersection);
        _bitmap.clear(intersection);
    }

#ifdef DEBUG
    void printUnrenderedPixels(const RectI& roi) const;
#endif

    /**
     * @brief Fills the image with the given colour. If the image components
     * are not RGBA it will ignore the unsupported components.
     * For example if the image comps is eImageComponentAlpha, then only the alpha value 'a' will
     * be used.
     **/
    void fill(const RectI & roi,float r,float g,float b,float a);

    void fillZero(const RectI& roi);

    void fillBoundsZero();

    /**
     * @brief Same as fill(const RectI&,float,float,float,float) but fills the R,G and B
     * components with the same value.
     **/
    void fill(const RectI & rect,
              float colorValue = 0.f,
              float alphaValue = 1.f)
    {
        fill(rect,colorValue,colorValue,colorValue,alphaValue);
    }

    /**
     * @brief Copies the content of the portion defined by roi of the other image pixels into this image.
     * The internal bitmap will be copied aswell
     **/
    void pasteFrom(const Image & src, const RectI & srcRoi, bool copyBitmap = true);

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

    /**
     * @brief Scales the roi of this image to the size of the output image.
     * This is used internally by buildMipMapLevel when the image is a NPOT.
     * This should not be used for downscaling.
     * The scale is computed from the RoD of both images.
     * FIXME: this following function has plenty of bugs (see code).
     **/
    void scaleBox(const RectI & roi, Image* output) const;


    static double getScaleFromMipMapLevel(unsigned int level);
    static unsigned int getLevelFromScale(double s);

    /**
     * @brief This function can be used to do the following conversion:
     * 1) RGBA to RGB
     * 2) RGBA to alpha
     * 3) RGB to RGBA
     * 4) RGB to alpha
     *
     * Also this function converts to the output bit depth.
     *
     * This function only works for images with the same region of definition and mipmaplevel.
     *
     *
     * @param renderWindow The rectangle to convert
     *
     * @param srcColorSpace Input data will be taken to be in this color-space
     *
     * @param dstColorSpace Output data will be converted to this color-space.
     *
     * @param channelForAlpha is used in cases 2) and 4) to determine from which channel we should
     * fill the alpha. If it is -1 it indicates you want to clear the mask.
     *
     * @param copyBitMap The bitmap will also be copied.
     *
     * @param requiresUnpremult If true, if a component conversion from RGBA to RGB happens
     * the RGB channels will be divided by the alpha channel when copied to the output image.
     *
     * Note that this function is mainly used for the following conversion:
     * RGBA --> Alpha
     * or bit depth conversion
     * Implementation should tend to optimize these cases.
     **/
    void convertToFormat(const RectI & renderWindow,
                         ViewerColorSpaceEnum srcColorSpace,
                         ViewerColorSpaceEnum dstColorSpace,
                         int channelForAlpha,
                         bool copyBitMap,
                         bool requiresUnpremult,
                         Image* dstImg) const;

    void convertToFormatAlpha0(const RectI & renderWindow,
                               ViewerColorSpaceEnum srcColorSpace,
                               ViewerColorSpaceEnum dstColorSpace,
                               int channelForAlpha,
                               bool copyBitMap,
                               bool requiresUnpremult,
                               Image* dstImg) const;

private:


    void convertToFormatCommon(const RectI & renderWindow,
                               ViewerColorSpaceEnum srcColorSpace,
                               ViewerColorSpaceEnum dstColorSpace,
                               int channelForAlpha,
                               bool useAlpha0,
                               bool copyBitMap,
                               bool requiresUnpremult,
                               Image* dstImg) const;

    template <typename PIX, bool doPremult>
    void premultInternal(const RectI& roi);
    template <bool doPremult>
    void premultForDepth(const RectI& roi);


public:


    void premultImage(const RectI& roi);
    void unpremultImage(const RectI& roi);

    bool canCallCopyUnProcessedChannels(std::bitset<4> processChannels) const;

    void copyUnProcessedChannels(const RectI& roi,
                                 ImagePremultiplicationEnum outputPremult,
                                 ImagePremultiplicationEnum originalImagePremult,
                                 std::bitset<4> processChannels,
                                 const boost::shared_ptr<Image>& originalImage);

    void applyMaskMix(const RectI& roi,
                      const Image* maskImg,
                      const Image* originalImg,
                      bool masked,
                      bool maskInvert,
                      float mix);

    /**
     * @brief returns true if image contains NaNs or infinite values, and fix them.
     */
    bool checkForNaNs(const RectI& roi) WARN_UNUSED_RETURN;

    void copyBitmapRowPortion(int x1, int x2,int y, const Image& other);

    void copyBitmapPortion(const RectI& roi, const Image& other);

    template <typename PIX>
    static PIX clamp(PIX x, PIX minval, PIX maxval);


    template<typename PIX>
    static PIX clampIfInt(float v);

    template <typename SRCPIX,typename DSTPIX>
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

    template <typename PIX, int maxValue, int srcNComps, int dstNComps, bool doR, bool doG, bool doB, bool doA, bool premult, bool originalPremult>
    void copyUnProcessedChannelsForPremult(std::bitset<4> processChannels,
                                           const RectI& roi,
                                           const boost::shared_ptr<Image>& originalImage);

    template <typename PIX, int maxValue, int srcNComps, int dstNComps>
    void copyUnProcessedChannelsForPremult(bool premult, bool originalPremult,
                                           std::bitset<4> processChannels,
                                           const RectI& roi,
                                           const boost::shared_ptr<Image>& originalImage);

    template <typename PIX, int maxValue, int srcNComps, int dstNComps, bool doR, bool doG, bool doB, bool doA>
    void copyUnProcessedChannelsForChannels(std::bitset<4> processChannels,
                                            bool premult,
                                            const RectI& roi,
                                            const boost::shared_ptr<Image>& originalImage,
                                            bool originalPremult);

    template <typename PIX, int maxValue, int srcNComps, int dstNComps>
    void copyUnProcessedChannelsForChannels(std::bitset<4> processChannels,
                                            bool premult,
                                            const RectI& roi,
                                            const boost::shared_ptr<Image>& originalImage,
                                            bool originalPremult);



    template <typename PIX, int maxValue,int srcNComps, int dstNComps>
    void copyUnProcessedChannelsForComponents(bool premult,
                                              const RectI& roi,
                                              std::bitset<4> processChannels,
                                              const boost::shared_ptr<Image>& originalImage,
                                              bool originalPremult);

    template <typename PIX, int maxValue>
    void copyUnProcessedChannelsForDepth(bool premult,
                                         const RectI& roi,
                                         std::bitset<4> processChannels,
                                         const boost::shared_ptr<Image>& originalImage,
                                         bool originalPremult);



    /**
     * @brief Given the output buffer,the region of interest and the mip map level, this
     * function computes the mip map of this image in the given roi.
     * If roi is NOT a power of 2, then it will be rounded to the closest power of 2.
     **/
    void buildMipMapLevel(const RectD& dstRoD,const RectI & roiCanonical, unsigned int level, bool copyBitMap,
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

    template <typename PIX,int maxValue>
    void upscaleMipMapForDepth(const RectI & roi, unsigned int fromLevel, unsigned int toLevel, Image* output) const;

    template<typename PIX>
    void pasteFromForDepth(const Image & src, const RectI & srcRoi, bool copyBitmap = true, bool takeSrcLock = true);

    template <typename PIX, int maxValue>
    void fillForDepth(const RectI & roi,float r,float g,float b,float a);

    template <typename PIX,int maxValue, int nComps>
    void fillForDepthForComponents(const RectI & roi_,  float r,float g, float b, float a);

    template<typename PIX>
    void scaleBoxForDepth(const RectI & roi, Image* output) const;

private:
    ImageBitDepthEnum _bitDepth;
    Bitmap _bitmap;
    RectD _rod;     // rod in canonical coordinates (not the same as the OFX::Image RoD, which is in pixel coordinates)
    RectI _bounds;
    double _par;
    bool _useBitmap;
};

//template <> inline unsigned char clamp(unsigned char v) { return v; }
//template <> inline unsigned short clamp(unsigned short v) { return v; }
template <>
inline float
Image::clamp(float x, float minval, float maxval)
{
    return std::min(std::max(x, minval), maxval);
}

template <>
inline double
Image::clamp(double x, double minval, double maxval)
{
    return std::min(std::max(x, minval), maxval);
}

template<> inline unsigned char Image::clampIfInt(float v) { return (unsigned char)clamp<float>(v, 0, 255); }
template<> inline unsigned short Image::clampIfInt(float v) { return (unsigned short)clamp<float>(v, 0, 65535); }
template<> inline float Image::clampIfInt(float v) { return v; }

NATRON_NAMESPACE_EXIT;

#endif // NATRON_ENGINE_IMAGE_H
