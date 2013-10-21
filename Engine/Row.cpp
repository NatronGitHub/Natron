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

#include "Row.h"

#include "Engine/Node.h"

using namespace Powiter;

/*Constructor used by the cache*/
Row::Row(const RowKey& key, size_t count, int cost, std::string path )
: CacheEntryHelper<float,RowKey>(key, count, cost, path)
{
    initializePlanesOffset();
}

Row::Row(const RowKey& key,const std::string& path)
: CacheEntryHelper<float,RowKey>(key,path)
{
    initializePlanesOffset();
}

Row::Row(int x,int y,int r,ChannelSet channels)
: CacheEntryHelper<float,RowKey>(makeKey(0,0, x, y, r, channels), (r-x)*channels.size(), 0)
{
    initializePlanesOffset();
}

void Row::initializePlanesOffset(){
    int count = 0;
    for(int i = 0; i <= POWITER_MAX_VALID_CHANNEL_INDEX ; ++i){
        if(i!=0 && _params._channels.contains((Channel)i)){
            _planesOffset[i] = _data.writable() + (width() * count);
            ++count;
        }else{
            _planesOffset[i] = NULL;
        }
        
    }
}
