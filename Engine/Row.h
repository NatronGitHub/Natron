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

#ifndef POWITER_ENGINE_ROW_H_
#define POWITER_ENGINE_ROW_H_

#include <string>


#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>

#include "Engine/ChannelSet.h"
#include "Engine/Cache.h"

#define POWITER_MAX_BUFFERS_PER_ROW 10

namespace Natron{
    
    class Image;
    
    class RowKey : public KeyHelper<U64>{
        
    public:
        
        RowKey()
		: KeyHelper<U64>()
        , _nodeHashKey(0)
        , _frameNb(0)
        , _left(0)
        , _y(0)
        , _right(0)
        , _channels()
        {}
        
        RowKey(KeyHelper<U64>::hash_type hash)
		: KeyHelper<U64>(hash)
        , _nodeHashKey(0)
        , _frameNb(0)
        , _left(0)
        , _y(0)
        , _right(0)
        , _channels()
        {}
        
        RowKey(U64 nodeHashKey,int frameNb,int x,int y,int r,ChannelSet channels)
        : KeyHelper<U64>()
        , _nodeHashKey(nodeHashKey)
        , _frameNb(frameNb)
        , _left(x)
        , _y(y)
        , _right(r)
        , _channels(channels)
        {}
        
        U64 _nodeHashKey;
        int _frameNb;
        int _left;
        int _y;
        int _right;
        ChannelSet _channels;
        
        void fillHash(Hash64* hash) const{
            hash->append(_nodeHashKey);
            hash->append(_frameNb);
            hash->append(_left);
            hash->append(_y);
            hash->append(_right);
            hash->append(_channels.value() >> 1); // removing mask_all
        }
        
        bool operator==(const RowKey& other) const {
            return _nodeHashKey == other._nodeHashKey &&
            _frameNb == other._frameNb && 
            _left == other._left &&
            _y == other._y &&
            _right == other._right &&
            _channels == other._channels;
        }
        
    };
}
namespace boost {
    namespace serialization {
        
        template<class Archive>
        void serialize(Archive & ar, Natron::RowKey & r, const unsigned int version)
        {
            (void)version;
            ar & r._nodeHashKey;
            ar & r._left;
            ar & r._y;
            ar & r._right;
            ar & r._channels;
        }
    }
}

namespace Natron{
    
    
    /* @brief The row defines 1 scan-line in an image.
     *It stores as many buffers (planes) as there're channels enabled for this row.
     *Note that channels are not default initialized and that the user is
     *responsible for filling them with an appropriate default value.
     **/
    class Row : public CacheEntryHelper<float,RowKey>
    {
        
    public:
        /*Constructor used by the cache*/
        Row(const RowKey& key, size_t count, int cost, std::string path = std::string()) ;
        
        Row(const RowKey& key,const std::string& path);
        
        Row(int left,int y,int right,ChannelSet channels);
        
        static RowKey makeKey(U64 nodeHashKey,int frameNb,int x,int y,int r,ChannelSet channels){
            return RowKey(nodeHashKey,frameNb,x,y,r,channels);
        }
        
        virtual ~Row() {}
       
        float* begin(Natron::Channel c){return _planesOffset[(int)c];}
        
        const float* begin(Natron::Channel c) const{return _planesOffset[(int)c];}

        float* end(Natron::Channel c) {return begin(c) + width();}
        
        const float* end(Natron::Channel c) const {return begin(c) + width();}

        /*set to 0s the entirety of the channel c*/
        void erase(Natron::Channel c){
            if (begin(c)) {
                std::fill(begin(c), end(c), 0.);
            }
        }
        
        void eraseAll() {
            foreachChannels(c, _params._channels) {
                std::fill(begin(c), end(c), 0.);
            }
        }
        
        void fill(Natron::Channel c,float fillValue) {
            std::fill(begin(c), end(c), fillValue);
        }
        
        // the user is responsible for locking the entry, using lock() and unlock()
        int y() const {return _params._y; }
        
        // the user is responsible for locking the entry, using lock() and unlock()
        int right() const {return _params._right;}
        
        // the user is responsible for locking the entry, using lock() and unlock()
        int left() const {return _params._left;}

        int width() const {return (_params._right - _params._left);}
        
        // the user is responsible for locking the entry, using lock() and unlock()
        const ChannelSet& channels() const { return _params._channels; }
        
    private:
        
        void initializePlanesOffset();
        
        float* _planesOffset[NATRON_MAX_VALID_CHANNEL_INDEX+1];
    };
    
    void copyRowToImage(const Natron::Row& row,int y,int x,Natron::Image* output);
    
    //function to convert row to image
    
}

#endif // POWITER_ENGINE_ROW_H_

