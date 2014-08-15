//  Natron
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 *Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 *contact: immarespond at gmail dot com
 *
 */

#ifndef NATRON_ENGINE_IMAGE_H_
#define NATRON_ENGINE_IMAGE_H_

#include <list>
#include <map>

#include "Global/GlobalDefines.h"

CLANG_DIAG_OFF(deprecated)
#include <QtCore/QHash>
CLANG_DIAG_ON(deprecated)
#include <QtCore/QReadWriteLock>

#include "Engine/CacheEntry.h"
#include "Engine/Rect.h"

namespace Natron {
    
    class ImageParams;
    
    class ImageKey :  public KeyHelper<U64>
    {
    public:
        
        U64 _nodeHashKey;
        SequenceTime _time;
        unsigned int _mipMapLevel;
        int _view;
        double _pixelAspect;

        ImageKey();
        
        ImageKey(U64 nodeHashKey,
                 SequenceTime time,
                 unsigned int mipMapLevel,
                 int view,
                 double pixelAspect = 1.);
        
        void fillHash(Hash64* hash) const;
        
        U64 getTreeVersion() const { return _nodeHashKey; }
        
        bool operator==(const ImageKey& other) const;
        
        SequenceTime getTime() const { return _time; }

    };
    
    
    class Bitmap {
    public:
        Bitmap(const RectI& bounds)
        : _bounds(bounds)
        , _map(bounds.area())
        {
            //Do not assert !rod.isNull() : An empty image can be created for entries that correspond to
            // "identities" images (i.e: images that are just a link to another image). See EffectInstance :
            // "!!!Note that if isIdentity is true it will allocate an empty image object with 0 bytes of data."
            //assert(!rod.isNull());
            clear();
        }
        
        Bitmap()
        : _bounds()
        , _map()
        {
            
        }
        
        void initialize(const RectI& bounds)
        {
            assert(_map.size() == 0);
            _bounds = bounds;
            _map.resize(_bounds.area());
            clear();
        }
        
        ~Bitmap() {}
        
        void clear() { std::fill(_map.begin(), _map.end(), 0); }
        
        const RectI& getBounds() const {return _bounds;}
        
        std::list<RectI> minimalNonMarkedRects(const RectI& roi) const;
        
        RectI minimalNonMarkedBbox(const RectI& roi) const;

        void markForRendered(const RectI& roi);

        const char* getBitmap() const { return _map.data(); }
        
        char* getBitmap() { return _map.data(); }
        
        const char* getBitmapAt(int x,int y) const;
        
        char* getBitmapAt(int x,int y);

    private:
        RectI _bounds;
        std::vector<char> _map;
    };
    

    
    
    class Image  : public CacheEntryHelper<unsigned char,ImageKey>
    {
    public:
        
        Image(const ImageKey& key,const boost::shared_ptr<const NonKeyParams>&  params,const Natron::CacheAPI* cache);
        
        
        /*This constructor can be used to allocate a local Image. The deallocation should
         then be handled by the user. Note that no view number is passed in parameter
         as it is not needed.*/
        Image(ImageComponents components,
              const RectD& regionOfDefinition, //!< rod in canonical coordinates
              const RectI& bounds, //!< bounds in pixel coordinates
              unsigned int mipMapLevel,
              Natron::ImageBitDepth bitdepth);
        
        virtual ~Image(){ deallocate(); }
#ifdef NATRON_DEBUG
        virtual void onMemoryAllocated() OVERRIDE FINAL;
#endif
        static ImageKey makeKey(U64 nodeHashKey,
                                SequenceTime time,
                                unsigned int mipMapLevel,
                                int view);
        
        static boost::shared_ptr<ImageParams> makeParams(int cost,
                                                         const RectD& rod, // the image rod in canonical coordinates
                                                         unsigned int mipMapLevel,
                                                         bool isRoDProjectFormat,
                                                         ImageComponents components,
                                                         Natron::ImageBitDepth bitdepth,
                                                         int inputNbIdentity,
                                                         int inputTimeIdentity,
                                                         const std::map<int, std::vector<RangeD> >& framesNeeded) ;
        
        /**
         * @brief Returns the region of definition of the image in canonical coordinates. It doesn't have any
         * scale applied to it. In order to return the true pixel data window you must call getBounds()
         * WARNING: this is NOT the same definition as in OpenFX, where the Image RoD is always in pixels.
         **/
        const RectD& getRoD() const { return _rod; };
        
        /**
         * @brief Returns the bounds where data is in the image.
         * This is equivalent to calling getRoD().mipMapLevel(getMipMapLevel());
         * but slightly faster since it is stored as a member of the image.
         **/
        const RectI& getBounds() const { return _bounds; };
        
        virtual size_t size() const OVERRIDE FINAL { return dataSize() + _bitmap.getBounds().area(); }
        
        unsigned int getMipMapLevel() const {return this->_key._mipMapLevel;}
                
        unsigned int getComponentsCount() const;
        
        ImageComponents getComponents() const {return this->_components;}
        
        /**
         * @brief This function returns true if the components 'from' have enough components to
         * convert to the 'to' components.
         * e.g: RGBA to RGB would return true , the opposite would return false.
         **/
        static bool hasEnoughDataToConvert(Natron::ImageComponents from,Natron::ImageComponents to);
        
        static std::string getFormatString(Natron::ImageComponents comps,Natron::ImageBitDepth depth);
        static std::string getDepthString(Natron::ImageBitDepth depth);
        
        static bool isBitDepthConversionLossy(Natron::ImageBitDepth from,Natron::ImageBitDepth to);
        
        Natron::ImageBitDepth getBitDepth() const {return this->_bitDepth;}
        
        void setPixelAspect(double pa) { this->_key._pixelAspect = pa; }
        
        double getPixelAspect() const { return this->_key._pixelAspect; }
        
        /**
         * @brief Access pixels. The pointer must be cast to the appropriate type afterwards.
         **/
        unsigned char* pixelAt(int x,int y);
        const unsigned char* pixelAt(int x,int y) const;
        
        /**
         * @brief Same as getElementsCount(getComponents()) * getBounds().width()
         **/
        unsigned int getRowElements() const;
        
        const char* getBitmapAt(int x,int y) const { return this->_bitmap.getBitmapAt(x,y); }
        
        char* getBitmapAt(int x,int y) { return this->_bitmap.getBitmapAt(x,y); }
        
        /**
         * @brief Zeroes out the bitmap so the image is considered to be as though nothing
         * had been rendered.
         **/
        void clearBitmap();

        /**
         * @brief Returns a list of portions of image that are not yet rendered within the 
         * region of interest given. This internally uses the bitmap to know what portion
         * are already rendered in the image. It aims to return the minimal
         * area to render. Since this problem is quite hard to solve,the different portions
         * of image returned may contain already rendered pixels.
         **/
        std::list<RectI> getRestToRender(const RectI& regionOfInterest) const{
            QReadLocker locker(&_lock);
            return _bitmap.minimalNonMarkedRects(regionOfInterest);
        }
        
        RectI getMinimalRect(const RectI& regionOfInterest) const{
            QReadLocker locker(&_lock);
            return _bitmap.minimalNonMarkedBbox(regionOfInterest);
        }

        void markForRendered(const RectI& roi){
            QWriteLocker locker(&_lock);
            _bitmap.markForRendered(roi);
        }
        
        
        
        /**
         * @brief Fills the image with the given colour. If the image components
         * are not RGBA it will ignore the unsupported components. 
         * For example if the image comps is ImageComponentAlpha, then only the alpha value 'a' will
         * be used.
         **/
        void fill(const RectI& roi,float r,float g,float b,float a);
        
        /**
         * @brief Same as fill(const RectI&,float,float,float,float) but fills the R,G and B
         * components with the same value.
         **/
        void fill(const RectI& rect,float colorValue = 0.f,float alphaValue = 1.f) {
            fill(rect,colorValue,colorValue,colorValue,alphaValue);
        }

        /**
         * @brief Fills the entire image with the given R,G,B value and an alpha value.
         **/
        void defaultInitialize(float colorValue = 0.f,float alphaValue = 1.f){
            fill(_bounds,colorValue,alphaValue);
        }
        
        /**
         * @brief Copies the content of the portion defined by roi of the other image pixels into this image.
         * The internal bitmap will be copied aswell
         **/
        void pasteFrom(const Natron::Image& src, const RectI& srcRoi, bool copyBitmap = true);
        
        /**
         * @brief Downscales a portion of this image into output.
         * This function will adjust roi to the largest enclosed rectangle for the
         * given mipmap level,
         * and then computes the mipmap of the given level of that rectangle.
         **/
        void downscaleMipMap(const RectI& roi, unsigned int fromLevel, unsigned int toLevel, Natron::Image* output) const;

        /**
         * @brief Upscales a portion of this image into output.
         * If the upscaled roi does not fit into output's bounds, it is cropped first.
         **/
        void upscaleMipMap(const RectI& roi, unsigned int fromLevel, unsigned int toLevel, Natron::Image* output) const;

        /**
         * @brief Scales the roi of this image to the size of the output image.
         * This is used internally by buildMipMapLevel when the image is a NPOT.
         * This should not be used for downscaling.
         * The scale is computed from the RoD of both images.
         * FIXME: this following function has plenty of bugs (see code).
         **/
        void scaleBox(const RectI& roi, Natron::Image* output) const;
        
        

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
         * @param invert If true the channels will be inverted when converting.
         *
         * @param copyBitMap The bitmap will also be copied.
         *
         * Note that this function is mainly used for the following conversion:
         * RGBA --> Alpha
         * or bit depth conversion
         * Implementation should tend to optimize these cases.
         **/
        void convertToFormat(const RectI& renderWindow,
                             Natron::ViewerColorSpace srcColorSpace,
                             Natron::ViewerColorSpace dstColorSpace,
                             int channelForAlpha,
                             bool invert,
                             bool copyBitMap,
                             Natron::Image* dstImg) const;



    private:
        
        /**
         * @brief Given the output buffer,the region of interest and the mip map level, this
         * function computes the mip map of this image in the given roi.
         * If roi is NOT a power of 2, then it will be rounded to the closest power of 2.
         **/
        void buildMipMapLevel(const RectI& roi, unsigned int level, Natron::Image* output) const;
        
        
        /**
         * @brief Halve the given roi of this image into output.
         * If the RoI bounds are odd, the largest enclosing RoI with even bounds will be considered.
         **/
        void halveRoI(const RectI& roi, Natron::Image* output) const;

        template <typename PIX, int maxValue>
        void halveRoIForDepth(const RectI& roi, Natron::Image* output) const;

        /**
         * @brief Same as halveRoI but for 1D only (either width == 1 or height == 1)
         **/
        void halve1DImage(const RectI& roi, Natron::Image* output) const;
        
        template <typename PIX, int maxValue>
        void halve1DImageForDepth(const RectI& roi, Natron::Image* output) const;

        template <typename PIX,int maxValue>
        void upscaleMipMapForDepth(const RectI& roi, unsigned int fromLevel, unsigned int toLevel, Natron::Image* output) const;

        template<typename PIX>
        void pasteFromForDepth(const Natron::Image& src, const RectI& srcRoi, bool copyBitmap = true);

        template <typename PIX, int maxValue>
        void fillForDepth(const RectI& roi,float r,float g,float b,float a);

        template<typename PIX>
        void scaleBoxForDepth(const RectI& roi, Natron::Image* output) const;

    private:
        Natron::ImageBitDepth _bitDepth;
        ImageComponents _components;
        mutable QReadWriteLock _lock;
        Bitmap _bitmap;
        RectD _rod; // rod in canonical coordinates (not the same as the OFX::Image RoD, which is in pixel coordinates)
        RectI _bounds;
    };

    template <typename SRCPIX,typename DSTPIX>
    DSTPIX convertPixelDepth(SRCPIX pix);
    
    template <typename PIX>
    PIX clamp(PIX v);

    template <typename PIX,int maxVal>
    PIX clampInternal(PIX v) {
        if (v > maxVal) return maxVal;
        if (v < 0) return 0;
        return v;
    }

    template <> inline unsigned char clamp(unsigned char v) { return clampInternal<unsigned char, 255>(v); }
    template <> inline unsigned short clamp(unsigned short v) { return clampInternal<unsigned short, 65535>(v); }
    template <> inline float clamp(float v) { return clampInternal<float, 1>(v); }
    template <> inline double clamp(double v) { return clampInternal<double, 1>(v); }
}//namespace Natron


#endif // NATRON_ENGINE_IMAGE_H_
