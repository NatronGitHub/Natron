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
#include <boost/scoped_array.hpp>

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
        RenderScale _renderScale;
        int _view;
        double _pixelAspect;

        ImageKey();
        
        ImageKey(U64 nodeHashKey,
                 SequenceTime time,
                 RenderScale scale,
                 int view,
                 double pixelAspect = 1.);
        
        void fillHash(Hash64* hash) const;
        
        
        bool operator==(const ImageKey& other) const;

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
            
            std::fill(_map.get(), _map.get()+rod.area(), 0); // is it necessary?
        }
        
        ~Bitmap() {}
        
        const RectI& getRoD() const {return _rod;}
        
        std::list<RectI> minimalNonMarkedRects(const RectI& roi) const;
        
        RectI minimalNonMarkedBbox(const RectI& roi) const;

        void markForRendered(const RectI& roi);

        const char* getBitmap() const { return _map.get(); }

    private:
        RectI _rod;
        boost::scoped_array<char> _map;
    };
    

    
    
    class Image  : public CacheEntryHelper<float,ImageKey>
    {
        
        ImageComponents _components;
        Bitmap _bitmap;
        mutable QReadWriteLock _lock;
        
    public:
   
        Image(const ImageKey& key,const NonKeyParams& params,bool restore,const std::string& path);
        
        
        /*This constructor can be used to allocate a local Image. The deallocation should
         then be handled by the user. Note that no view number is passed in parameter
         as it is not needed.*/
        Image(ImageComponents components,const RectI& regionOfDefinition,RenderScale scale,SequenceTime time);
        
        virtual ~Image(){}
        
        static ImageKey makeKey(U64 nodeHashKey,
                                SequenceTime time,
                                RenderScale scale,
                                int view);
        
        static boost::shared_ptr<ImageParams> makeParams(int cost,const RectI& rod,bool isRoDProjectFormat,ImageComponents components,
                                                         int inputNbIdentity,int inputTimeIdentity,
                                                         const std::map<int, std::vector<RangeD> >& framesNeeded) ;
        
        const RectI& getRoD() const {return _bitmap.getRoD();}
        
        virtual size_t size() const OVERRIDE FINAL { return dataSize() + _bitmap.getRoD().area(); }
        
        RenderScale getRenderScale() const {return this->_key._renderScale;}
        
        SequenceTime getTime() const {return this->_key._time;}
        
        ImageComponents getComponents() const {return this->_components;}
        
        void setPixelAspect(double pa) { this->_key._pixelAspect = pa; }
        
        double getPixelAspect() const { return this->_key._pixelAspect; }
        
        float* pixelAt(int x,int y);
        
        const float* pixelAt(int x,int y) const;
        
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
            fill(_bitmap.getRoD(),colorValue,alphaValue);
        }
        
        /**
         * @brief Copies the content of the other image pixels into this image.
         **/
        void copy(const Natron::Image& other);
        
        /**
         * @brief Makes a scaled copy of this image into output.
         * @pre Output is an image of (this->getRoD().width() * sx,this->getRoD().height() * sy)
         * @param sx The scale to apply in the x dimension. It must be 1 divided by a power of 2.
         * @param sy The scale to apply in the y dimension. It must be 1 divided by a power of 2.
         * 
         * The method use is quite simple: we just average 4 pixels.
         *
         **/
        void scale(Natron::Image* output,double sx,double sy) const;
    
    
    };    
    
}//namespace Natron


#endif // NATRON_ENGINE_IMAGE_H_
