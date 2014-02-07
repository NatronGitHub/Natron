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

#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/scoped_array.hpp>

#include <QtCore/QHash>
#include <QtCore/QMutex>

#include "Engine/Cache.h"
#include "Engine/Rect.h"

namespace Natron{
    
    class ImageKey :  public KeyHelper<U64>
    {
        
        
    public:
        
        U64 _nodeHashKey;
        SequenceTime _time;
        RenderScale _renderScale;
        RectI _rod;
        int _view;
        double _pixelAspect;

        ImageKey()
		: KeyHelper<U64>()
        , _nodeHashKey(0)
        , _time(0)
        , _renderScale()
        , _rod()
        , _view(0)
        , _pixelAspect(1)
        {}

        
        ImageKey(int cost,U64 nodeHashKey,SequenceTime time,RenderScale scale,int view,const RectI& regionOfDefinition,double pixelAspect = 1.)
		: KeyHelper<U64>(cost,regionOfDefinition.area() * 4) //< images are only RGBA for now hence the 4
        , _nodeHashKey(nodeHashKey)
        , _time(time)
        , _rod(regionOfDefinition)
        , _view(view)
        , _pixelAspect(pixelAspect)
        { _renderScale = scale; }
        
        void fillHash(Hash64* hash) const{
            hash->append(_nodeHashKey);
            hash->append(_renderScale.x);
            hash->append(_renderScale.y);
            hash->append(_time);
            hash->append(_view);
            hash->append(_pixelAspect);
        }
        
        bool operator==(const ImageKey& other) const {
            return _nodeHashKey == other._nodeHashKey &&
            _renderScale.x == other._renderScale.x &&
            _renderScale.y == other._renderScale.y &&
            // _rod == other._rod &&
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
        Image(const RectI& regionOfDefinition,RenderScale scale,SequenceTime time)
        : CacheEntryHelper<float,ImageKey>(makeKey(0,0,time,scale,0,regionOfDefinition),false,"")
        , _bitmap(regionOfDefinition)
        {
        }
        
        virtual ~Image(){}
        
        static ImageKey makeKey(int cost,U64 nodeHashKey,SequenceTime time,RenderScale scale,int view,const RectI& regionOfDefinition){
            return ImageKey(cost,nodeHashKey,time,scale,view,regionOfDefinition);
        }
        
        const RectI& getRoD() const {return _bitmap.getRoD();}
        
        RenderScale getRenderScale() const {return this->_params._renderScale;}
        
        SequenceTime getTime() const {return this->_params._time;}
        
        void setPixelAspect(double pa) { this->_params._pixelAspect = pa; }
        
        double getPixelAspect() const { return this->_params._pixelAspect; }
        
        float* pixelAt(int x,int y){
            const RectI& rod = _bitmap.getRoD();
            return this->_data.writable() + (y-rod.bottom())*4*rod.width() + (x-rod.left())*4;
        }
        
        const float* pixelAt(int x,int y) const{
            const RectI& rod = _bitmap.getRoD();
            return this->_data.readable() + (y-rod.bottom())*4*rod.width() + (x-rod.left())*4;
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
        
        
        
        void fill(const RectI& rect,float r,float g,float b,float a){
            for(int i = rect.bottom(); i < rect.top();++i){
                float* dst = pixelAt(rect.left(),i);
                for(int j = 0; j < rect.width();++j){
                    dst[j*4] = r;
                    dst[j*4+1] = g;
                    dst[j*4+2] = b;
                    dst[j*4+3] = a;
                }
            }
        }
        
        void fill(const RectI& rect,float colorValue = 0.f,float alphaValue = 1.f){
            fill(rect,colorValue,colorValue,colorValue,alphaValue);
        }

        
        void defaultInitialize(float colorValue = 0.f,float alphaValue = 1.f){
            fill(_bitmap.getRoD(),colorValue,alphaValue);
        }
        
        void copy(const Natron::Image& other);
    
    
    };
    /*Useful function that saves on disk the image in png format.
     The name of the image will be the hash key of the image.*/
    void debugImage(Natron::Image* img,const QString& filename = QString());
    
    
}//namespace Natron
#endif // NATRON_ENGINE_IMAGE_H_
