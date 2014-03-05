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

#ifndef NATRON_ENGINE_FRAMEENTRY_H_
#define NATRON_ENGINE_FRAMEENTRY_H_

#include <QtCore/QObject>


#include "Global/GlobalDefines.h"

#include "Engine/CacheEntry.h"
#include "Engine/ChannelSet.h"
#include "Engine/Format.h"
#include "Engine/TextureRect.h"


class Hash64;

namespace Natron{
    
    class FrameParams;
    
    class FrameKey : public KeyHelper<U64> {
    public:
        SequenceTime _time;
        U64 _treeVersion;
        double _exposure;
        int _lut;
        int _bitDepth;
        int _channels;
        int _view;
        TextureRect _textureRect; // texture rectangle definition (bounds in the original image + width and height)
        
        
        FrameKey();
        
        
        FrameKey(SequenceTime time,U64 treeVersion,double exposure,
                 int lut,int bitDepth,int channels,int view,const TextureRect& textureRect);
        
        void fillHash(Hash64* hash) const;
        
        bool operator==(const FrameKey& other) const ;
    };
  
}


namespace Natron {
    class FrameEntry : public CacheEntryHelper<U8,FrameKey>
    {
        
    public:
        

        
        FrameEntry(const FrameKey& key,const NonKeyParams& params,bool restore,const std::string& path)
        :CacheEntryHelper<U8,FrameKey>(key,params,restore,path)
        {
        }
      
        ~FrameEntry(){}
        
        static FrameKey makeKey(SequenceTime time,U64 treeVersion,double exposure,
                                int lut,int bitDepth,int channels,int view,const TextureRect& textureRect);
        static boost::shared_ptr<const FrameParams> makeParams(const RectI rod,int bitDepth,int texW,int texH);
        
        U8* data() const {return _data.writable();}
        
    };
    
    
}


#endif // NATRON_ENGINE_FRAMEENTRY_H_
