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

#include "Global/Macros.h"
CLANG_DIAG_OFF(unused-parameter)
// /opt/local/include/boost/serialization/smart_cast.hpp:254:25: warning: unused parameter 'u' [-Wunused-parameter]
#include <boost/archive/binary_iarchive.hpp>
CLANG_DIAG_ON(unused-parameter)
#include <boost/archive/binary_oarchive.hpp>
#include <boost/scoped_array.hpp>

CLANG_DIAG_OFF(deprecated)
#include <QtCore/QHash>
CLANG_DIAG_ON(deprecated)
#include <QtCore/QMutex>

#include "Engine/Cache.h"
#include "Engine/Rect.h"

namespace Natron {
    
    enum ImageComponents {
        ImageComponentNone = 0,
        ImageComponentAlpha,
        ImageComponentRGB,
        ImageComponentRGBA
    };
    
    class ImageKey :  public KeyHelper<U64>
    {
        
        
    public:
        
        ImageComponents _components;
        U64 _nodeHashKey;
        SequenceTime _time;
        RenderScale _renderScale;
        RectI _rod;
        int _view;
        double _pixelAspect;

        ImageKey()
		: KeyHelper<U64>()
        , _components(ImageComponentNone)
        , _nodeHashKey(0)
        , _time(0)
        , _renderScale()
        , _rod()
        , _view(0)
        , _pixelAspect(1)
        {}

        
        ImageKey(int cost,
                 U64 nodeHashKey,
                 SequenceTime time,
                 RenderScale scale,
                 int view,
                 ImageComponents components,
                 const RectI& regionOfDefinition,
                 double pixelAspect = 1.)
		: KeyHelper<U64>(cost,regionOfDefinition.area() * getElementsCountForComponents(components)) //< images are only RGBA for now hence the 4
        , _components(components)
        , _nodeHashKey(nodeHashKey)
        , _time(time)
        , _rod(regionOfDefinition)
        , _view(view)
        , _pixelAspect(pixelAspect)
        { _renderScale = scale; }
        
        void fillHash(Hash64* hash) const {
            hash->append(_components);
            hash->append(_nodeHashKey);
            hash->append(_renderScale.x);
            hash->append(_renderScale.y);
            hash->append(_time);
            hash->append(_view);
            hash->append(_pixelAspect);
        }
        
        static int getElementsCountForComponents(ImageComponents comp) {
            switch (comp) {
                case ImageComponentNone:
                    return 0;
                case ImageComponentAlpha:
                    return 1;
                case ImageComponentRGB:
                    return 3;
                case ImageComponentRGBA:
                    return 4;
                default:
                    ///unsupported components
                    assert(false);
                    break;
            }
        }
        
        bool operator==(const ImageKey& other) const {
            return _components == other._components &&
            _nodeHashKey == other._nodeHashKey &&
            _renderScale.x == other._renderScale.x &&
            _renderScale.y == other._renderScale.y &&
            // _rod == other._rod && //< do not compare the rod (@see renderRoI)
            _time == other._time &&
            _view == other._view &&
            _pixelAspect == other._pixelAspect;
            
        }
        
        friend class boost::serialization::access;
        template<class Archive>
        void serialize(Archive & ar, const unsigned int version)
        {
            (void)version;
            ar & _rod;
            ar & _components;
            ar & _nodeHashKey;
            ar & _renderScale.x;
            ar & _renderScale.y;
            ar & _time;
            ar & _view;
            ar & _pixelAspect;
        }
    };
    
    class Bitmap{
    public:
        Bitmap(const RectI& rod)
        : _rod(rod)
        , _map(new char[rod.area()])
        {
            std::fill(_map.get(), _map.get()+rod.area(), 0); // is it necessary?
        }
        
        ~Bitmap() {}
        
        const RectI& getRoD() const {return _rod;}
        
        std::list<RectI> minimalNonMarkedRects(const RectI& roi) const;
        
        void markForRendered(const RectI& roi);

        const char* getBitmap() const { return _map.get(); }

    private:
        RectI minimalNonMarkedBbox(const RectI& roi) const;

        RectI _rod;
        boost::scoped_array<char> _map;
    };
    

    
    
    class Image  : public CacheEntryHelper<float,ImageKey>
    {
        
        Bitmap _bitmap;
        mutable QMutex _lock;
        
    public:
   
        Image(const ImageKey& key,bool restore,const std::string& path):
        CacheEntryHelper<float,ImageKey>(key,restore,path)
        ,_bitmap(key._rod){
        }
        
        /*This constructor can be used to allocate a local Image. The deallocation should
         then be handled by the user. Note that no view number is passed in parameter
         as it is not needed.*/
        Image(ImageComponents components,const RectI& regionOfDefinition,RenderScale scale,SequenceTime time)
        : CacheEntryHelper<float,ImageKey>(makeKey(0,0,time,scale,0,components,regionOfDefinition),false,"")
        , _bitmap(regionOfDefinition)
        {
        }
        
        virtual ~Image(){}
        
        static ImageKey makeKey(int cost,
                                U64 nodeHashKey,
                                SequenceTime time,
                                RenderScale scale,
                                int view,
                                ImageComponents components,
                                const RectI& regionOfDefinition){
            return ImageKey(cost,nodeHashKey,time,scale,view,components,regionOfDefinition);
        }
        
        const RectI& getRoD() const {return _bitmap.getRoD();}
        
        RenderScale getRenderScale() const {return this->_params._renderScale;}
        
        SequenceTime getTime() const {return this->_params._time;}
        
        ImageComponents getComponents() const {return this->_params._components;}
        
        void setPixelAspect(double pa) { this->_params._pixelAspect = pa; }
        
        double getPixelAspect() const { return this->_params._pixelAspect; }
        
        float* pixelAt(int x,int y){
            const RectI& rod = _bitmap.getRoD();
            int compsCount = ImageKey::getElementsCountForComponents(getComponents());
            return this->_data.writable() + (y-rod.bottom()) * compsCount * rod.width() + (x-rod.left()) * compsCount;
        }
        
        const float* pixelAt(int x,int y) const {
            const RectI& rod = _bitmap.getRoD();
            int compsCount = ImageKey::getElementsCountForComponents(getComponents());
            return this->_data.readable() + (y-rod.bottom()) * compsCount * rod.width() + (x-rod.left()) * compsCount;
        }
        /**
         * @brief Returns a list of portions of image that are not yet rendered within the 
         * region of interest given. This internally uses the bitmap to know what portion
         * are already rendered in the image. It aims to return the minimal
         * area to render. Since this problem is quite hard to solve,the different portions
         * of image returned may contain already rendered pixels.
         **/
        std::list<RectI> getRestToRender(const RectI& regionOfInterest) const{
            QMutexLocker locker(&_lock);
            return _bitmap.minimalNonMarkedRects(regionOfInterest);
        }
        
        void markForRendered(const RectI& roi){
            QMutexLocker locker(&_lock);
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
    
    
    };    
    
}//namespace Natron
#endif // NATRON_ENGINE_IMAGE_H_
