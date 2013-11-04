//  Powiter
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 *Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 *contact: immarespond at gmail dot com
 *
 */

#ifndef IMAGE_H
#define IMAGE_H

#include <list>

#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>

#include <QtCore/QMutex>

#include "Engine/Cache.h"
#include "Engine/RectI.h"

namespace Powiter{
    
    class ImageKey :  public KeyHelper<U64>
    {
        
        
    public:
        
        U64 _nodeHashKey;
        SequenceTime _time;
        RenderScale _renderScale;
        RectI _rod;
        int _view;

        ImageKey()
		: KeyHelper<U64>()
        , _nodeHashKey(0)
        , _time(0)
        , _renderScale()
        , _rod()
        , _view(0)
        {}
        
        ImageKey(KeyHelper<U64>::hash_type hash)
		: KeyHelper<U64>(hash)
        , _nodeHashKey(0)
        , _time(0)
        , _renderScale()
        , _rod()
        , _view(0)
        {}
        
        ImageKey(U64 nodeHashKey,SequenceTime time,RenderScale scale,int view,const RectI& regionOfDefinition)
		: KeyHelper<U64>()
        , _nodeHashKey(nodeHashKey)
        , _time(time)
        , _rod(regionOfDefinition)
        , _view(view)
        { _renderScale = scale; }
        
        
        void fillHash(Hash64* hash) const{
            hash->append(_nodeHashKey);
            hash->append(_renderScale.x);
            hash->append(_renderScale.y);
            hash->append(_time);
            hash->append(_view);
        }
        
        bool operator==(const ImageKey& other) const {
            return _nodeHashKey == other._nodeHashKey &&
            _renderScale.x == other._renderScale.x &&
            _renderScale.y == other._renderScale.y &&
            // _rod == other._rod &&
            _time == other._time &&
            _view == other._view;
            
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
        }
    };
    
    class Bitmap{
        
        char* _map;
        RectI _rod;
        int _pixelsRenderedCount;
    public:
        
        Bitmap(const RectI& rod):_rod(rod),_pixelsRenderedCount(0){
            _map = (char*)calloc(rod.area(),sizeof(char));
        }
        
        ~Bitmap(){ free(_map); }
        
        const RectI& getRoD() const {return _rod;}
        
        std::list<RectI> minimalNonMarkedRects(const RectI& roi) const;
        
        void markForRendered(const RectI& roi);
    };
    

    
    
    class Image  : public CacheEntryHelper<float,ImageKey>
    {
        
        Bitmap _bitmap;
        mutable QMutex _lock;
        
    public:
        Image(const ImageKey& key, size_t count, int cost, std::string path = std::string()):
        CacheEntryHelper<float,ImageKey>(key,count,cost,path)
        ,_bitmap(key._rod){
        }
        
        Image(const ImageKey& key,const std::string& path):
        CacheEntryHelper<float,ImageKey>(key,path)
        ,_bitmap(key._rod){
        }
        
        /*This constructor can be used to allocate a local Image. The deallocation should
         then be handled by the user. Note that no view number is passed in parameter
         as it is not needed.*/
        Image(const RectI& regionOfDefinition,RenderScale scale,SequenceTime time):
        CacheEntryHelper<float,ImageKey>(makeKey(0,time,scale,0,regionOfDefinition)
                                            ,regionOfDefinition.width()*regionOfDefinition.height()*4
                                            , 0)
        ,_bitmap(regionOfDefinition)
        {
        }
        
        virtual ~Image(){}
        
        static ImageKey makeKey(U64 nodeHashKey,SequenceTime time,RenderScale scale,int view,const RectI& regionOfDefinition){
            return ImageKey(nodeHashKey,time,scale,view,regionOfDefinition);
        }
        
        const RectI& getRoD() const {return _bitmap.getRoD();}
        
        RenderScale getRenderScale() const {return this->_params._renderScale;}
        
        SequenceTime getTime() const {return this->_params._time;}
        
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
        
        void fill(const RectI& rect,float colorValue = 0.f,float alphaValue = 1.f){
            for(int i = rect.bottom(); i < rect.top();++i){
                float* dst = pixelAt(rect.left(),i);
                for(int j = 0; j < rect.width();++j){
                    dst[j*4] = colorValue;
                    dst[j*4+1] = colorValue;
                    dst[j*4+2] = colorValue;
                    dst[j*4+3] = alphaValue;
                }
            }
        }
        
        void defaultInitialize(float colorValue = 0.f,float alphaValue = 1.f){
            fill(_bitmap.getRoD(),colorValue,alphaValue);
        }
        
    };
    /*Useful function that saves on disk the image in png format.
     The name of the image will be the hash key of the image.*/
    void debugImage(Powiter::Image* img);
    
}//namespace Powiter
#endif // IMAGE_H
