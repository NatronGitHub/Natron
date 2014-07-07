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
        Bitmap(const RectI& rod)
        : _rod(rod)
        , _map(new char[rod.area()])
        {
            //Do not assert !rod.isNull() : An empty image can be created for entries that correspond to
            // "identities" images (i.e: images that are just a link to another image). See EffectInstance :
            // "!!!Note that if isIdentity is true it will allocate an empty image object with 0 bytes of data."
            //assert(!rod.isNull());
            clear();
        }
        
        Bitmap()
        : _rod()
        , _map(0)
        {
            
        }
        
        void initialize(const RectI& rod)
        {
            assert(!_map);
            _rod = rod;
            _map = new char[rod.area()];
            clear();
        }
        
        ~Bitmap() { delete [] _map; }
        
        void clear() { std::fill(_map, _map+ _rod.area(), 0); }
        
        const RectI& getRoD() const {return _rod;}
        
        std::list<RectI> minimalNonMarkedRects(const RectI& roi) const;
        
        RectI minimalNonMarkedBbox(const RectI& roi) const;

        void markForRendered(const RectI& roi);

        const char* getBitmap() const { return _map; }
        
        char* getBitmap() { return _map; }
        
        const char* getBitmapAt(int x,int y) const;
        
        char* getBitmapAt(int x,int y);

    private:
        RectI _rod;
        char* _map;
    };
    

    
    
    class Image  : public CacheEntryHelper<unsigned char,ImageKey>
    {
        
        Natron::ImageBitDepth _bitDepth;
        ImageComponents _components;
        mutable QReadWriteLock _lock;
        Bitmap _bitmap;
        RectI _rod;
        RectI _pixelRod;

        
    public:
   
        Image(const ImageKey& key,const boost::shared_ptr<const NonKeyParams>&  params,const Natron::CacheAPI* cache);
        
        
        /*This constructor can be used to allocate a local Image. The deallocation should
         then be handled by the user. Note that no view number is passed in parameter
         as it is not needed.*/
        Image(ImageComponents components,const RectI& regionOfDefinition,unsigned int mipMapLevel,Natron::ImageBitDepth bitdepth);
        
        virtual ~Image(){}
#ifdef NATRON_DEBUG
        virtual void onMemoryAllocated() OVERRIDE FINAL;
#endif
        static ImageKey makeKey(U64 nodeHashKey,
                                SequenceTime time,
                                unsigned int mipMapLevel,
                                int view);
        
        static boost::shared_ptr<ImageParams> makeParams(int cost,const RectI& rod,unsigned int mipMapLevel,
                                                         bool isRoDProjectFormat,ImageComponents components,
                                                         Natron::ImageBitDepth bitdepth,
                                                         int inputNbIdentity,int inputTimeIdentity,
                                                         const std::map<int, std::vector<RangeD> >& framesNeeded) ;
        
        /**
         * @brief Returns the region of definition of the image in canonical coordinates. It doesn't have any
         * scale applied to it. In order to return the true pixel data window you must call getPixelRoD()
         **/
        const RectI& getRoD() const;
        
        /**
         * @brief Returns the bounds where data is in the image.
         * This is equivalent to calling getRoD().mipMapLevel(getMipMapLevel());
         * but slightly faster since it is stored as a member of the image.
         **/
        const RectI& getPixelRoD() const;
        
        virtual size_t size() const OVERRIDE FINAL { return dataSize() + _bitmap.getRoD().area(); }
        
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
         * @brief Same as getElementsCount(getComponents()) * getPixelRoD().width()
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
        void fill(const RectI& rect,float r,float g,float b,float a);
        
        /**
         * @brief Same as fill(const RectI&,float,float,float,float) but fills the R,G and B
         * components with the same value.
         **/
        void fill(const RectI& rect,float colorValue = 0.f,float alphaValue = 1.f){
            fill(rect,colorValue,colorValue,colorValue,alphaValue);
        }

        /**
         * @brief Fills the entire image with the given R,G,B value and an alpha value.
         **/
        void defaultInitialize(float colorValue = 0.f,float alphaValue = 1.f){
            fill(_pixelRod,colorValue,alphaValue);
        }
        
        /**
         * @brief Copies the content of the portion defined by roi of the other image pixels into this image.
         * The internal bitmap will be copied aswell
         **/
        void copy(const Natron::Image& other,const RectI& roi,bool copyBitmap = true);
        
        /**
         * @brief Downscales a portion of this image into output.
         * This function will adjust roi to the largest enclosed rectangle for the
         * given mipmap level,
         * and then computes the mipmap of the given level of that rectangle.
         **/
        void downscale_mipmap(const RectI& roi, Natron::Image* output, unsigned int level) const;

        /**
         * @brief Upscales a portion of this image into output.
         **/
        void upscale_mipmap(const RectI& roi, Natron::Image* output, unsigned int level) const;

        /**
         * @brief Scales the roi of this image to the size of the output image.
         * This is used internally by buildMipMapLevel when the image is a NPOT.
         * This should not be used for downscaling.
         **/
        void scale_box_generic(const RectI& roi,Natron::Image* output) const;
        
        

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
        void convertToFormat(const RectI& renderWindow,Natron::Image* dstImg,
                             Natron::ViewerColorSpace srcColorSpace,
                             Natron::ViewerColorSpace dstColorSpace,
                             int channelForAlpha,bool invert,bool copyBitMap) const;
        

        
    private:
        
        /**
         * @brief Given the output buffer,the region of interest and the mip map level, this
         * function computes the mip map of this image in the given roi.
         * If roi is NOT a power of 2, then it will be rounded to the closest power of 2.
         **/
        void buildMipMapLevel(Natron::Image* output,const RectI& roi,unsigned int level) const;
        
        
        /**
         * @brief Halve the given roi of this image into output.
         * If the RoI bounds are odd, the largest enclosing RoI with even bounds will be considered.
         **/
        void halveRoI(const RectI& roi,Natron::Image* output) const;
        
        /**
         * @brief Same as halveRoI but for 1D only (either width == 1 or height == 1)
         **/
        void halve1DImage(const RectI& roi,Natron::Image* output) const;
    };
    
    template <typename SRCPIX,typename DSTPIX>
    DSTPIX convertPixelDepth(SRCPIX pix);
    
    template <typename PIX>
    PIX clamp(PIX v);
    
}//namespace Natron


#endif // NATRON_ENGINE_IMAGE_H_
